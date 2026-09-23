"""Shared semantic encoders and a normalized, exactly partitioned DayIntent decoder."""
from dataclasses import asdict, dataclass

import torch
from torch import nn
from torch.nn import functional as F


@dataclass(frozen=True)
class Config:
    width: int = 128
    depth: int = 2
    board: bool = False
    history: bool = True
    dropout: float = 0.
    financial: bool = False
    coordinated: bool = False
    accounting: bool = False
    space_mask: bool = False
    product_plans: bool = False


def mlp(input_width, output_width, width, depth=2, dropout=0.):
    layers = []
    for _ in range(depth - 1):
        layers += [nn.Linear(input_width, width), nn.ReLU()]
        if dropout:
            layers += [nn.Dropout(dropout)]
        input_width = width
    return nn.Sequential(*layers, nn.Linear(input_width, output_width))


def pool(value, mask):
    weight = mask.float().unsqueeze(-1)
    mean = (value * weight).sum(-2) / weight.sum(-2).clamp_min(1)
    maximum = value.masked_fill(~mask.unsqueeze(-1), -1e4).amax(-2)
    maximum = torch.where(mask.any(-1, keepdim=True), maximum, torch.zeros_like(maximum))
    return torch.cat((mean, maximum), -1)


def mask_logits(logits, minimum, maximum):
    values = torch.arange(101, device=logits.device)
    return logits.float().masked_fill((values < minimum.unsqueeze(-1)) | (values > maximum.unsqueeze(-1)), -1e9)


class Policy(nn.Module):
    # Decode land before population targets; all outputs still map to the fixed API.
    ORDER = (8, 5, 6, 7, 0, 1, 2, 3, 4)

    def __init__(self, config=Config()):
        super().__init__()
        self.config = config
        w = config.width
        self.global_encoder = mlp(48 + 16 * config.financial + 9 * config.accounting, w, w, config.depth)
        self.product_encoder = mlp(32 + 9, w, w, config.depth)
        self.crop_encoder = mlp(24, w, w, config.depth)
        self.animal_encoder = mlp(24, w, w, config.depth)
        if config.board:
            self.board_encoder = mlp(36, w, w, config.depth)
        self.fusion = mlp((11 if config.board else 7) * w, w, 2 * w, config.depth, config.dropout)
        self.crop_decoder = mlp(2 * w + 12 + 12 + 2 + (17 + 60 * config.product_plans) * config.coordinated, 101, w, config.depth)
        self.animal_decoder = mlp(2 * w + 12 + 60 * config.product_plans, 101, w, config.depth)
        self.global_decoder = mlp(w + 12 + 60 * config.product_plans + 24 + 18 + 9, 101, w, config.depth)
        self.register_buffer("product_identity", torch.eye(9))
        self.register_buffer("factor_identity", torch.eye(9))
        order = torch.tensor(self.ORDER)
        positions = torch.argsort(order)
        self.register_buffer("prefix_mask", positions[None, :] < positions[:, None])

    @staticmethod
    def financial_features(batch):
        g, p = batch['global'].float(), batch['products'].float()
        cash = torch.expm1(g[:, 4].abs() * 10) * g[:, 4].sign()
        prices = torch.expm1(p[:, :, 0].abs() * 10) * p[:, :, 0].sign()
        size = batch['animal_size'].float()
        animals = size.sum(1)
        endangered = (size * batch['animal_forced']).sum(1)
        wheat = (p[:, 0, 2] + p[:, 0, 3]) * 100
        feed_cost = (animals-wheat).clamp_min(0) * prices[:, 0]
        survival_cost = (endangered-wheat).clamp_min(0) * prices[:, 0]
        goods = ((p[:, 1:8, 2]+p[:, 1:8, 3]) * 100 * prices[:, 1:8]).sum(1)
        values = [cash/100, cash/1000, cash/10000, cash/100000,
                  cash/(prices[:, 0].clamp_min(1)*100), animals/100, endangered/100, wheat/100,
                  feed_cost/1000, (cash-feed_cost)/1000, survival_cost/1000, (cash-survival_cost)/1000,
                  cash/300, cash/400, cash/500, goods/10000]
        return torch.stack(values, -1).clamp(-10, 10)

    def group_plans(self, batch, target):
        options = batch['crop_options'].float()
        weighted = options * target.unsqueeze(-1)
        output = (target * options[..., 9] * options[..., 5] * 6).sum(-1)
        output = output.unsqueeze(-1) * batch['crop_key'][..., :5]
        parts = [weighted.sum(-2), output]
        if self.config.product_plans:
            parts.append((batch['crop_key'][..., :5, None] * weighted.sum(-2).unsqueeze(-2)).flatten(-2))
        return torch.cat(parts, -1) / 100.

    def crop_summary(self, batch, target):
        weighted = (batch['crop_options'].float() * target.unsqueeze(-1)).sum(-2)
        parts = [weighted.sum(1)]
        if self.config.product_plans:
            parts.append((batch['crop_key'][..., :5, None] * weighted.unsqueeze(-2)).sum(1).flatten(1))
        return torch.cat(parts, -1) / 100.

    @staticmethod
    def accounting_features(batch):
        area = batch['global'][:, 6].float() * 100
        crop_size, animal_size = batch['crop_size'].float(), batch['animal_size'].float()
        crops, animals = crop_size.sum(1), animal_size.sum(1)
        mature = (crop_size * (batch['crop_options'][..., 9].amax(-1) > 0)).sum(1)
        ongoing = (crop_size * batch['crop_key'][..., 2:4].sum(-1)).sum(1)
        species = [(animal_size * batch['animal_species'].eq(s)).sum(1) for s in range(3)]
        return torch.stack([area, area-crops-animals, crops, animals, mature, ongoing, *species], -1)/100.

    @staticmethod
    def available_sites(batch, crop_values, globals_values):
        options = batch['crop_options']
        released = (crop_values * (options[..., 0]+options[..., 9])).sum((1, 2))
        return (batch['global'][:, 6]*100).round().long() - batch['crop_size'].sum(1) - batch['animal_size'].sum(1) + released.round().long() + 25*globals_values[:, 8]

    def encode(self, batch):
        products = batch["products"].float()
        if not self.config.history:
            products = products * torch.cat((torch.ones(24, device=products.device), torch.zeros(8, device=products.device)))
        products = self.product_encoder(torch.cat((products, self.product_identity.expand(len(products), -1, -1)), -1))
        crops = self.crop_encoder(batch["crop_key"].float())
        animals = self.animal_encoder(batch["animal_key"].float())
        global_input = batch['global'].float()
        if self.config.financial:
            global_input = torch.cat((global_input, self.financial_features(batch)), -1)
        if self.config.accounting:
            global_input = torch.cat((global_input, self.accounting_features(batch)), -1)
        parts = [self.global_encoder(global_input), products.mean(1), products.amax(1),
                 pool(crops, batch["crop_size"] > 0), pool(animals, batch["animal_size"] > 0)]
        if self.config.board:
            board = self.board_encoder(batch["board"].float())
            parts += [torch.cat((board.mean(2), board.amax(2)), -1).flatten(1)]
        return self.fusion(torch.cat(parts, -1)), crops, animals

    def crop_logits(self, context, encoded, options, previous, used, size, maximum, future, prior=None):
        shape = options.shape[:-1]
        inputs = [context[:, None, None].expand(*shape, -1),
            encoded[:, :, None].expand(*shape, -1), options, previous / 100.,
            used.unsqueeze(-1) / 100., (size.unsqueeze(-1) - used).unsqueeze(-1) / 100.]
        if self.config.coordinated:
            inputs.append(prior[:, :, None].expand(*shape, -1))
        logits = self.crop_decoder(torch.cat(inputs, -1))
        remaining = size.unsqueeze(-1) - used
        upper = torch.minimum(remaining, maximum)
        lower = (remaining - future).clamp_min(0)
        return mask_logits(logits, lower, upper)

    @staticmethod
    def summaries(batch, crop_values, serve):
        crop_plan = (batch["crop_options"] * crop_values.unsqueeze(-1)).sum((1, 2)) / 100.
        animal_plan = (batch["animal_key"] * serve.unsqueeze(-1)).sum(1) / 100.
        escape = torch.where(batch["animal_forced"].bool(), batch["animal_size"] - serve, torch.zeros_like(serve))
        retained = torch.stack([((batch["animal_size"] - escape) * batch["animal_species"].eq(s)).sum(1) for s in range(3)], -1)
        return crop_plan, animal_plan, retained, escape

    def forward(self, batch):
        context, crop_encoded, animal_encoded = self.encode(batch)
        target = batch["crop_target"].long()
        used = target.cumsum(-1) - target
        options = batch["crop_options"].float()
        weighted = options * target.unsqueeze(-1)
        previous = weighted.cumsum(-2) - weighted
        maximum = batch["crop_maximum"].long()
        future = maximum.sum(-1, keepdim=True) - maximum.cumsum(-1)
        group_plans = self.group_plans(batch, target) if self.config.coordinated else None
        prior = group_plans.cumsum(1)-group_plans if self.config.coordinated else None
        crops = self.crop_logits(context, crop_encoded, options, previous, used, batch["crop_size"].long(), maximum, future, prior)
        crop_plan = self.crop_summary(batch, target)
        size = batch["animal_size"].long()
        raw = self.animal_decoder(torch.cat((context[:, None].expand(-1, size.shape[1], -1), animal_encoded,
                                            crop_plan[:, None].expand(-1, size.shape[1], -1)), -1))
        upper = torch.where(batch["day"][:, None] == 29, torch.zeros_like(size), size)
        animals = mask_logits(raw, torch.zeros_like(size), upper)
        _, animal_plan, retained, _ = self.summaries(batch, target, batch["animal_serve"].long())
        globals_target = batch["globals"].long()
        prefix = globals_target[:, None].float() * self.prefix_mask[None] / 100.
        known = self.prefix_mask.float().expand(len(context), -1, -1)
        raw = self.global_decoder(torch.cat((context[:, None].expand(-1, 9, -1), crop_plan[:, None].expand(-1, 9, -1),
            animal_plan[:, None].expand(-1, 9, -1), prefix, known, self.factor_identity.expand(len(context), -1, -1)), -1))
        lower = torch.zeros_like(globals_target)
        lower[:, 5:8] = retained
        upper = torch.full_like(globals_target, 100)
        upper[:, 8] = (batch["global"][:, 6] < 1).long()
        for p in range(5):
            upper[:, p] -= globals_target[:, :p].sum(-1)
        if self.config.space_mask:
            sites = self.available_sites(batch, target, globals_target)
            births = globals_target[:, 5:8]-retained
            for s in range(3):
                upper[:, 5+s] = torch.minimum(upper[:, 5+s], retained[:, s]+sites-births[:, :s].sum(-1))
            for p in range(5):
                upper[:, p] = torch.minimum(upper[:, p], sites-births.sum(-1)-globals_target[:, :p].sum(-1))
        globals_logits = mask_logits(raw, lower, upper)
        return {"crop": crops, "animal": animals, "global": globals_logits}

    @torch.no_grad()
    def predict(self, batch, temperature=0.):
        def choose(logits):
            if temperature <= 0:
                return logits.argmax(-1)
            return torch.distributions.Categorical(logits=logits / temperature).sample()
        context, crop_encoded, animal_encoded = self.encode(batch)
        options = batch["crop_options"].float()
        maximum = batch["crop_maximum"].long()
        size = batch["crop_size"].long()
        crop_values = torch.zeros_like(maximum)
        prior = torch.zeros((len(context), 1, 17+60*self.config.product_plans), device=context.device)
        groups = range(size.shape[1]) if self.config.coordinated else (None,)
        for group in groups:
            section = slice(group, group+1) if group is not None else slice(None)
            local_options, local_maximum, local_size = options[:, section], maximum[:, section], size[:, section]
            previous = torch.zeros_like(local_options[:, :, :1])
            used = torch.zeros_like(local_size[:, :, None])
            for index in range(options.shape[2]):
                logits = self.crop_logits(context, crop_encoded[:, section], local_options[:, :, index:index+1], previous, used, local_size,
                                         local_maximum[:, :, index:index+1], local_maximum[:, :, index+1:].sum(-1, keepdim=True), prior)
                chosen = choose(logits)
                crop_values[:, section, index:index+1] = chosen
                used = used + chosen
                previous = previous + local_options[:, :, index:index+1] * chosen.unsqueeze(-1)
            if self.config.coordinated:
                contribution = (local_options * crop_values[:, section].unsqueeze(-1)).sum(-2)/100.
                output = (crop_values[:, section] * local_options[..., 9] * local_options[..., 5] * 6).sum(-1)
                output = output.unsqueeze(-1) * batch['crop_key'][:, section, :5] / 100.
                parts = [contribution, output]
                if self.config.product_plans:
                    parts.append((batch['crop_key'][:, section, :5, None] * contribution.unsqueeze(-2)).flatten(-2))
                prior = prior + torch.cat(parts, -1)
        crop_plan = self.crop_summary(batch, crop_values)
        size = batch["animal_size"].long()
        raw = self.animal_decoder(torch.cat((context[:, None].expand(-1, size.shape[1], -1), animal_encoded,
                                            crop_plan[:, None].expand(-1, size.shape[1], -1)), -1))
        upper = torch.where(batch["day"][:, None] == 29, torch.zeros_like(size), size)
        serve = choose(mask_logits(raw, torch.zeros_like(size), upper))
        _, animal_plan, retained, escape = self.summaries(batch, crop_values, serve)
        global_values = torch.zeros((len(context), 9), device=context.device, dtype=torch.long)
        known = torch.zeros_like(global_values)
        for factor in self.ORDER:
            identity = self.factor_identity[factor].expand(len(context), -1)
            raw = self.global_decoder(torch.cat((context, crop_plan, animal_plan, global_values.float()/100., known.float(), identity), -1))
            lower = retained[:, factor-5] if 5 <= factor < 8 else torch.zeros_like(serve[:, 0])
            upper = torch.full_like(lower, 100)
            if factor == 8:
                upper = (batch["global"][:, 6] < 1).long()
            elif factor < 5:
                upper -= global_values[:, :factor].sum(-1)
            if self.config.space_mask and factor != 8:
                sites = self.available_sites(batch, crop_values, global_values)
                if 5 <= factor < 8:
                    born = (global_values[:, 5:factor]-retained[:, :factor-5]).sum(-1)
                    limit = retained[:, factor-5]+sites-born
                else:
                    born = (global_values[:, 5:8]-retained).sum(-1)
                    limit = sites-born-global_values[:, :factor].sum(-1)
                upper = torch.minimum(upper, limit)
            global_values[:, factor] = choose(mask_logits(raw, lower, upper))
            known[:, factor] = 1
        return dict(globals=global_values, crop=crop_values, serve=serve, escape=escape)

    def config_dict(self):
        return asdict(self.config)


def loss(output, batch, ordinal=0., positive_weight=1.):
    families = (("global", "globals", torch.ones_like(batch["globals"], dtype=torch.bool)),
                ("crop", "crop_target", batch["crop_maximum"] > 0),
                ("animal", "animal_serve", (batch["animal_size"] > 0) & (batch["day"][:, None] < 29)))
    total = output["global"].sum() * 0.
    stats = {}
    for family, name, active in families:
        logits, target = output[family], batch[name].long()
        # Forced factors carry no learnable choice and must not dilute the loss.
        active = active & (logits > -1e8).sum(-1).gt(1)
        weights = active.float() * torch.where(target > 0, positive_weight, 1.)
        per = F.cross_entropy(logits.flatten(0, -2), target.flatten(), reduction="none").reshape(target.shape)
        nll = (per * weights).sum() / weights.sum().clamp_min(1)
        if ordinal:
            values = torch.arange(101, device=logits.device)
            expected = (logits.softmax(-1) * values).sum(-1)
            nll = nll + ordinal * (F.smooth_l1_loss(expected, target.float(), reduction="none") * weights).sum() / weights.sum().clamp_min(1)
        total = total + nll
        prediction = logits.argmax(-1)
        denominator = active.sum().clamp_min(1)
        stats[family+"_nll"] = nll.detach()
        stats[family+"_accuracy"] = ((prediction == target) & active).sum() / denominator
        stats[family+"_mae"] = ((prediction-target).abs() * active).sum() / denominator
        stats[family+"_count"] = active.sum()
        stats[family+"_weight"] = weights.sum()
    stats["loss"] = total.detach()
    return total, stats
