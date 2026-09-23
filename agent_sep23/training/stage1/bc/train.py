"""GPU-resident BC batches with measured throughput and frozen data identities."""
import argparse
import hashlib
import json
import math
import random
import time
from dataclasses import asdict
from pathlib import Path

import numpy as np
import torch

from bc.model import Config, Policy, loss

ROOT = Path(__file__).resolve().parents[1]
INPUTS = ("global", "products", "board", "crop_key", "crop_options", "crop_size", "crop_maximum", "crop_target",
          "animal_key", "animal_size", "animal_species", "animal_forced", "animal_serve", "animal_escape", "globals", "day")


def source_hash():
    digest = hashlib.sha256()
    for path in sorted((ROOT / "bc").glob("*.py")):
        digest.update(path.name.encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def load_data(path, device, config, rank=200, team=0, clean=False, submission=0):
    path = Path(path)
    values = {p.stem: np.load(p, mmap_mode="r") for p in path.glob("*.npy")}
    selected = (values["rank"] <= rank) & (values["error"] == 0) & (values["split"] < 3)
    if team:
        selected &= values["team"] == team
    if submission:
        selected &= values['submission'] == submission
    if clean:
        selected &= (values["missing"] == 0) & (values["projections"] == 0)
    data = {}
    for key in INPUTS:
        if key == "board" and not config.board:
            continue
        data[key] = torch.from_numpy(np.array(values[key][selected], copy=True)).to(device)
    splits = values["split"][selected]
    train = torch.as_tensor(np.flatnonzero(splits == 0), device=device)
    validation = torch.as_tensor(np.flatnonzero(splits == 1), device=device)
    if not len(train) or not len(validation):
        raise ValueError("Training and validation episodes are both required")
    identities = {name: values[name][selected].tolist() for name in ("episode", "seat", "day", "submission", "team", "split")}
    return data, train, validation, identities


def take(data, indices):
    return {key: value[indices] for key, value in data.items()}


@torch.no_grad()
def validate_contract(model, data, batch_size=128):
    n = len(data["day"])
    for start in range(0, n, batch_size):
        batch = {key: value[start:start+batch_size] for key, value in data.items()}
        output = model(batch)
        for family, target in (("crop", "crop_target"), ("animal", "animal_serve"), ("global", "globals")):
            supported = output[family].gather(-1, batch[target].long().unsqueeze(-1)).squeeze(-1) > -1e8
            if not supported.all().item():
                failing = (~supported).nonzero().cpu().tolist()[:12]
                raise ValueError(f"Unsupported {family} labels starting at {start}: {failing}")


@torch.no_grad()
def evaluate(model, data, indices, batch_size, amp):
    model.eval()
    sums = {}
    for start in range(0, len(indices), batch_size):
        chosen = indices[start:start+batch_size]
        batch = take(data, chosen)
        with torch.autocast("cuda", dtype=torch.bfloat16, enabled=amp):
            _, metrics = loss(model(batch), batch)
        add_metrics(sums, metrics)
    return finish_metrics(sums)


def add_metrics(sums, metrics):
    for family in ("global", "crop", "animal"):
        for suffix in ("nll", "accuracy", "mae", "count", "weight"):
            key = family + "_" + suffix
            value = metrics[key]
            if suffix in ("accuracy", "mae"):
                value = value * metrics[family + "_count"]
            elif suffix == "nll":
                value = value * metrics[family + "_weight"]
            sums[key] = sums.get(key, 0) + value


def finish_metrics(sums):
    result = {}
    for family in ("global", "crop", "animal"):
        for suffix in ("nll", "accuracy", "mae", "count", "weight"):
            key = family + "_" + suffix
            value = sums[key]
            if suffix in ("accuracy", "mae"):
                value = value / sums[family + "_count"].clamp_min(1)
            elif suffix == "nll":
                value = value / sums[family + "_weight"].clamp_min(1)
            result[key] = float(value)
    result["loss"] = sum(result[f + "_nll"] for f in ("global", "crop", "animal"))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--width", type=int, default=128)
    parser.add_argument("--depth", type=int, default=2)
    parser.add_argument("--board", action="store_true")
    parser.add_argument('--financial', action='store_true')
    parser.add_argument('--coordinated', action='store_true')
    parser.add_argument('--accounting', action='store_true')
    parser.add_argument('--space-mask', action='store_true')
    parser.add_argument('--product-plans', action='store_true')
    parser.add_argument("--no-history", action="store_true")
    parser.add_argument("--dropout", type=float, default=0.)
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--batch", type=int, default=256)
    parser.add_argument("--lr", type=float, default=.001)
    parser.add_argument("--weight-decay", type=float, default=.001)
    parser.add_argument("--ordinal", type=float, default=0.)
    parser.add_argument("--positive-weight", type=float, default=1.)
    parser.add_argument("--rank", type=int, default=10)
    parser.add_argument("--team", type=int, default=0)
    parser.add_argument('--submission', type=int, default=0)
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--seed", type=int, default=2301)
    parser.add_argument("--cpu-threads", type=int, default=4)
    parser.add_argument("--amp", action="store_true")
    parser.add_argument("--matmul-precision", choices=("highest", "high", "medium"), default="highest")
    parser.add_argument("--no-bf16-reduced-reduction", action="store_true")
    parser.add_argument("--compile", action="store_true")
    parser.add_argument("--initialize", type=Path)
    parser.add_argument("--save-every", type=int, default=10)
    args = parser.parse_args()
    torch.set_num_threads(args.cpu_threads)
    torch.manual_seed(args.seed)
    np.random.seed(args.seed)
    random.seed(args.seed)
    torch.set_float32_matmul_precision(args.matmul_precision)
    torch.backends.cuda.matmul.allow_bf16_reduced_precision_reduction = not args.no_bf16_reduced_reduction
    device = torch.device("cuda")
    config = Config(args.width, args.depth, args.board, not args.no_history, args.dropout, args.financial, args.coordinated,
                    args.accounting, args.space_mask, args.product_plans)
    args.run.mkdir(parents=True, exist_ok=False)
    data, training, validation, identities = load_data(args.data, device, config, args.rank, args.team, args.clean, args.submission)
    model = Policy(config).to(device)
    if args.initialize:
        checkpoint = torch.load(args.initialize, map_location=device, weights_only=False)
        if Config(**checkpoint["config"]) != config:
            raise ValueError("Fine-tuning requires the same architecture")
        model.load_state_dict(checkpoint["model"])
    validate_contract(model, data)
    settings = {key: str(value) if isinstance(value, Path) else value for key, value in vars(args).items()}
    contract = dict(config=asdict(config), settings=settings, source_hash=source_hash(),
                    torch_version=torch.__version__, cuda_version=torch.version.cuda,
                    gpu=torch.cuda.get_device_name(),
                    matmul_precision=torch.get_float32_matmul_precision(),
                    bf16_reduced_reduction=torch.backends.cuda.matmul.allow_bf16_reduced_precision_reduction,
                    dataset=json.loads((args.data / "summary.json").read_text()),
                    compiler=json.loads((ROOT / "snapshots/compiler_current.json").read_text())["source_hash"],
                    parameters=sum(p.numel() for p in model.parameters()),
                    training_dawns=len(training), validation_dawns=len(validation),
                    selected_identities_sha256=hashlib.sha256(json.dumps(identities, sort_keys=True).encode()).hexdigest())
    (args.run / "contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    (args.run / "selected_identities.json").write_text(json.dumps(identities) + "\n")
    print(json.dumps(contract), flush=True)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.lr, weight_decay=args.weight_decay, fused=True)
    forward = torch.compile(model, dynamic=False) if args.compile else model
    best, elapsed = math.inf, 0.
    torch.cuda.synchronize()
    overall = time.perf_counter()
    with (args.run / "metrics.jsonl").open("w") as log:
        for epoch in range(1, args.epochs + 1):
            model.train()
            order = training[torch.randperm(len(training), device=device)]
            start = time.perf_counter()
            sums, norms = {}, []
            # Keep the final update comparable in size; never overweight a tiny tail.
            for chosen in order.tensor_split(math.ceil(len(order) / args.batch)):
                batch = take(data, chosen)
                optimizer.zero_grad(set_to_none=True)
                with torch.autocast("cuda", dtype=torch.bfloat16, enabled=args.amp):
                    objective, metrics = loss(forward(batch), batch, args.ordinal, args.positive_weight)
                objective.backward()
                norm = torch.nn.utils.clip_grad_norm_(model.parameters(), 1.)
                if not torch.isfinite(norm):
                    raise FloatingPointError("Non-finite BC gradient")
                norms.append(norm.detach())
                optimizer.step()
                add_metrics(sums, metrics)
            torch.cuda.synchronize()
            seconds = time.perf_counter() - start
            elapsed += seconds
            if not all(torch.isfinite(p).all().item() for p in model.parameters()):
                raise FloatingPointError("Non-finite BC parameter")
            # Deployment uses FP32. Rank checkpoints using the same precision.
            previous_precision = torch.get_float32_matmul_precision()
            torch.set_float32_matmul_precision("highest")
            result = evaluate(model, data, validation, args.batch, False)
            torch.set_float32_matmul_precision(previous_precision)
            norms = torch.stack(norms)
            record = dict(epoch=epoch, train=finish_metrics(sums), validation=result,
                          gradient_norm_mean=float(norms.mean()), gradient_norm_max=float(norms.max()),
                          clipped_update_fraction=float((norms > 1).float().mean()),
                          train_seconds=seconds, train_dawns_per_second=len(training)/seconds,
                          total_seconds=time.perf_counter()-overall, peak_gpu_bytes=torch.cuda.max_memory_allocated())
            log.write(json.dumps(record) + "\n")
            log.flush()
            print(json.dumps(record), flush=True)
            checkpoint = dict(model=model.state_dict(), config=asdict(config), epoch=epoch, contract=contract,
                              validation=result, optimizer=optimizer.state_dict())
            if result["loss"] < best:
                best = result["loss"]
                torch.save(checkpoint, args.run / "best.pt")
            if epoch % args.save_every == 0 or epoch == args.epochs:
                torch.save(checkpoint, args.run / f"epoch_{epoch:03d}.pt")
        torch.save(checkpoint, args.run / "last.pt")


if __name__ == "__main__":
    main()
