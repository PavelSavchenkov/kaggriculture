# SPDX-License-Identifier: Apache-2.0
"""Join an existing own-fill ledger to T12 history, without invoking an actor.

All dependencies are injected existing components. Final-action binding and
quantity reconciliation belong to ObservedFillLedger; conservation/floor bounds
belong to SORREL; memory and complete seasonal windows belong to FlowHistory.
This module only joins those contracts. No action, game, projection or scenario
selection is performed here. Create a fresh bridge/ledger/history per match.
"""
from __future__ import annotations

from copy import deepcopy
import hashlib
import json
from typing import Any, Mapping


def _clock(observation: Mapping[str, Any], period: int) -> int:
    def count(value: Any) -> int:
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise ValueError('invalid observation clock')
        return value
    if 'step' in observation:
        step = count(observation['step'])
        if 'day' in observation and 'hour' in observation:
            if step != count(observation['day']) * period + count(observation['hour']):
                raise ValueError('inconsistent observation clock')
        return step
    return count(observation['day']) * period + count(observation['hour'])


def _action_hash(action: Any) -> str:
    return hashlib.sha256(json.dumps(action, sort_keys=True, separators=(',', ':'),
                                     allow_nan=False).encode()).hexdigest()


class SelectedActionHistory:
    """One actor's final-action -> observed fills -> public-flow history bridge.

    ``ledger`` is an existing ObservedFillLedger, ``history`` an existing
    FlowHistory, ``interval_type`` the existing FlowInterval, and ``infer`` the
    existing SORREL infer_rival_flow. ``mechanics`` supplies market_price,
    PRODUCTS, SHOPS and TOWN_CENTER_PRODUCTS. None is copied or reimplemented.

    Use record()/observe() when this bridge owns the ledger calls. A shared
    continuation consumer can instead call bind() with its existing record
    binding and observe(..., fill_result=result) with its existing observation
    result. That path makes NO extra ledger call. Both forms require the final
    queue and the SAME final unit-stage snapshots, supplied by the caller.
    """
    def __init__(self, *, ledger, history, interval_type, infer, mechanics):
        self.ledger, self.history = ledger, history
        self.interval_type, self.infer, self.mechanics = interval_type, infer, mechanics
        self.pending = None
        self.player = None
        self.last_consumed_step = -1

    def _prepare(self, observation, configuration, final_action):
        cfg = dict(configuration or {})
        period = max(1, int(cfg.get('turnsPerDay', 24)))
        if period != self.history.period:
            raise ValueError('history period differs; use the matching per-match history')
        cfg['turnsPerDay'] = period
        cfg['maxMarketOrdersPerTurn'] = max(1, int(cfg.get('maxMarketOrdersPerTurn', 10)))
        now = _clock(observation, period)
        player = observation['player']
        if isinstance(player, bool) or not isinstance(player, int) or player not in (0, 1):
            raise ValueError('expected player 0 or 1')
        if self.player is not None and player != self.player:
            raise ValueError('use a separate bridge for each actor')
        if now <= self.last_consumed_step:
            raise ValueError('consumed history cannot be rebound; use a new bridge for a new match')
        # Retain public inference inputs only; no rival private data, score,
        # random seed, future action, or observer-owned mutable reference.
        before = {'step': now, 'player': player,
                  'market': deepcopy(observation['market']),
                  'town': deepcopy(observation.get('town', {}))}
        action = json.loads(json.dumps(final_action, allow_nan=False))
        used_cfg = {key: deepcopy(cfg[key]) for key in (
            'turnsPerDay', 'maxMarketOrdersPerTurn', 'shedCapacity',
            'townShopSellInterval', 'townCenterSellInterval') if key in cfg}
        return before, used_cfg, action

    def bind(self, observation, configuration, final_action, binding):
        """Use a binding already returned by the caller's ledger.record()."""
        before, cfg, action = self._prepare(observation, configuration, final_action)
        expected = {'step': before['step'], 'player': before['player'],
                    'action_sha256': _action_hash(action)}
        if binding.get('status') != 'recorded' or any(binding.get(k) != v for k, v in expected.items()):
            raise ValueError('final action does not match its ledger binding')
        self.pending = {'before': before, 'configuration': cfg,
                        'action': action, 'binding': expected}
        self.player = before['player']
        return deepcopy(binding)

    def record(self, observation, configuration, final_action, *, post_unit_shed,
               post_unit_inventories=None):
        """Delegate once to the existing ledger, then bind the same final action."""
        self._prepare(observation, configuration, final_action)
        binding = self.ledger.record(observation, configuration, final_action,
            post_unit_shed=post_unit_shed, post_unit_inventories=post_unit_inventories)
        return self.bind(observation, configuration, final_action, binding)

    def observe(self, observation, *, fill_result=None):
        """Append completed prior-turn intervals; return unknowns without zeros.

        A supplied result is consumed, not recomputed. Exact quantities are
        summed only when EVERY valid sale slot for that product is singleton.
        Operating-product buy/sell ambiguity never becomes a FlowHistory sale.
        Floor-censored intervals stay intervals even when own fills are exact.
        """
        p = self.pending
        def outcome(status, reason, **extra):
            return {'status': status, 'reason': reason, 'intervals': [],
                    'own_sale_units': {}, 'cash_receipts': None, **extra}
        if p is None:
            return outcome('unknown', 'no_pending_action')
        binding = p['binding']
        try:
            now = _clock(observation, p['configuration']['turnsPerDay'])
            if observation['player'] != binding['player']:
                return outcome('unknown', 'different_player', binding=deepcopy(binding))
        except (KeyError, TypeError, ValueError, OverflowError):
            return outcome('unknown', 'invalid_observation', binding=deepcopy(binding))
        if now == binding['step']:
            return outcome('pending', 'same_observation_step', binding=deepcopy(binding))
        if now < binding['step']:
            return outcome('unknown', 'older_observation', binding=deepcopy(binding))
        result = self.ledger.observe(observation) if fill_result is None else deepcopy(fill_result)
        if result.get('binding') != binding:
            return outcome('unknown', 'fill_binding_mismatch', binding=deepcopy(binding))
        self.pending = None
        self.last_consumed_step = binding['step']
        if now != binding['step'] + 1:
            return outcome('unknown', 'nonadjacent_observation', binding=deepcopy(binding))
        if result.get('status') not in ('reconciled', 'ambiguous'):
            return outcome('unknown', 'own_fills_unavailable', binding=deepcopy(binding),
                           fill_status=result.get('status'), fill_reason=result.get('reason'))
        try:
            after = {'step': now, 'market': deepcopy(observation['market']),
                     'town': deepcopy(observation.get('town', {}))}
            before, cfg = p['before'], p['configuration']
            if before['market'].get('params') != after['market'].get('params'):
                return outcome('unknown', 'market_parameters_changed', binding=deepcopy(binding))
            products = tuple(self.mechanics.PRODUCTS)
            if any(item not in before['market']['inventory'] or item not in after['market']['inventory']
                   for item in products):
                return outcome('unknown', 'incomplete_market_observation', binding=deepcopy(binding))
            # The ledger already parsed and truncated the submitted queue. Keep
            # actual slot positions, using PASS for ignored/non-shed orders.
            slots = cfg['maxMarketOrdersPerTurn']
            queue = [['PASS'] for _ in range(slots)]
            sales = {item: 0 for item in products}
            buys = {item: 0 for item in ('WHEAT', 'FERTILIZER')}
            unknown_sales, unknown_buys = set(), set()
            for row in result['orders']:
                slot = row['slot']
                if row['kind'] not in ('sell', 'buy') or not 0 <= slot < slots:
                    continue
                item, op = row['item'], row['type']
                if item not in products:
                    continue  # Animal purchases never enter a product-flow claim.
                queue[slot] = [op, item, row['requested']]
                target, uncertain = (sales, unknown_sales) if op == 'SELL' else (buys, unknown_buys)
                if item not in target:
                    continue
                lo, hi = row['fill_min'], row['fill_max']
                if lo is None or hi is None or lo != hi:
                    uncertain.add(item)
                else:
                    target[item] += lo
            for item in unknown_sales:
                sales.pop(item, None)
            for item in unknown_buys:
                buys.pop(item, None)
            receipt = self.infer(before, after, queue, cfg,
                quote=lambda item, inv: self.mechanics.market_price(item, inv, before['market'].get('params')),
                shops=self.mechanics.SHOPS, center_products=self.mechanics.TOWN_CENTER_PRODUCTS,
                own_sale_units=sales, own_buy_units=buys)
            intervals, excluded = [], {}
            for item in products:
                row = receipt.get('products', {}).get(item, {})
                if item in ('WHEAT', 'FERTILIZER'):
                    excluded[item] = 'operating_product_buy_sell_ambiguity'
                elif row.get('status') != 'identified_interval':
                    excluded[item] = row.get('status', 'unavailable')
                else:
                    lo, hi = row['rival_sale_units_range']
                    a, b = row['rival_market_supply_units_range']
                    reason = 'floor_censored' if row['floor_nonadmission_possible'] else 'identified'
                    intervals.append(self.interval_type(binding['step'], item, lo, hi, a, b, reason))
        except (KeyError, TypeError, ValueError, OverflowError):
            return outcome('unknown', 'flow_inputs_unavailable', binding=deepcopy(binding))
        # No partially appended record when inference itself is unavailable.
        for interval in intervals:
            self.history.add(interval)
        return {'status': 'recorded' if intervals else 'unknown',
                'reason': 'observed_prior_flow' if intervals else 'no_identified_products',
                'binding': deepcopy(binding), 'observed_at': now,
                'fill_status': result['status'], 'own_sale_units': sales,
                'own_buy_units': buys, 'intervals': [i.as_dict() for i in intervals],
                'excluded': excluded, 'cash_receipts': None,
                'interpretation': 'past conditional flow bounds; no rival order or future probability'}
