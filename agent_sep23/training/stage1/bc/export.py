"""Export immutable CPU weights without a framework dependency at execution."""
import argparse
import struct
from pathlib import Path

import torch
from torch import nn

from bc.model import Config, Policy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument('--space-mask', action='store_true', help='Decode-only constraint ablation; does not retrain weights')
    args = parser.parse_args()
    checkpoint = torch.load(args.checkpoint, map_location="cpu", weights_only=False)
    config = Config(**checkpoint["config"])
    model = Policy(config).eval()
    model.load_state_dict(checkpoint["model"])
    modules = [(name, module) for name, module in model.named_children() if isinstance(module, nn.Sequential)]
    with args.output.open("wb") as stream:
        if config.product_plans:
            stream.write(b'BCW23004')
            stream.write(struct.pack('<10I', config.width, config.depth, config.board, config.history,
                                    config.financial, config.coordinated, config.accounting, config.space_mask or args.space_mask,
                                    config.product_plans, len(modules)))
        elif config.accounting or config.space_mask or args.space_mask:
            stream.write(b'BCW23003')
            stream.write(struct.pack('<9I', config.width, config.depth, config.board, config.history,
                                    config.financial, config.coordinated, config.accounting, config.space_mask or args.space_mask, len(modules)))
        elif config.financial or config.coordinated:
            stream.write(b'BCW23002')
            stream.write(struct.pack('<7I', config.width, config.depth, config.board, config.history,
                                    config.financial, config.coordinated, len(modules)))
        else:
            stream.write(b"BCW23001")
            stream.write(struct.pack("<5I", config.width, config.depth, config.board, config.history, len(modules)))
        for name, module in modules:
            layers = [layer for layer in module if isinstance(layer, nn.Linear)]
            name = name.encode()
            stream.write(struct.pack("<I", len(name)))
            stream.write(name)
            stream.write(struct.pack("<I", len(layers)))
            for layer in layers:
                stream.write(struct.pack("<II", layer.in_features, layer.out_features))
                stream.write(layer.weight.detach().numpy().astype("<f4").tobytes())
                stream.write(layer.bias.detach().numpy().astype("<f4").tobytes())
    print(args.output, args.output.stat().st_size)


if __name__ == "__main__":
    main()
