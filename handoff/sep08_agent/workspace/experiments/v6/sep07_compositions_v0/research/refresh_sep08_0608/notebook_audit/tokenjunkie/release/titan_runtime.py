# SPDX-License-Identifier: Apache-2.0
"""TITAN: one production selection, one explicit selected-action consumer."""
from copy import deepcopy
from dataclasses import dataclass
import importlib.util
from pathlib import Path
import time

HERE = Path(__file__).resolve().parent


_MODULE_CACHE = {}


def load(name, path, *, cache=False):
    """Cache only completed stateless modules; cancellation never publishes one.

    Controller instances and route/seed ledgers are constructed separately.
    Cache keys include the resolved artifact path to isolate relocated packages.
    """
    import sys
    key = (name, str(Path(path).resolve()))
    if cache and key in _MODULE_CACHE:
        return _MODULE_CACHE[key]
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    missing = object()
    previous = sys.modules.get(name, missing)
    try:
        sys.modules[name] = module
        spec.loader.exec_module(module)
        if cache:
            _MODULE_CACHE[key] = module
    except BaseException:
        _MODULE_CACHE.pop(key, None)
        if sys.modules.get(name) is module:
            if previous is missing:
                sys.modules.pop(name, None)
            else:
                sys.modules[name] = previous
        raise
    return module


deadline = load('_titan_deadline', HERE/'reference/titan-current/deadline_adapter.py')


@dataclass(frozen=True)
class Features:
    consumer: str = 'frozen'
    seed: bool = True
    funding: bool = True
    terminal_route: bool = False
    committed: bool = True
    budget_seconds: float = 1.0
    reserve_seconds: float = 0.01
    terminal_history: bool = False
    history_hypotheses: dict | None = None
    terminal_tie_break: str = 'baseline'

    def __post_init__(self):
        if self.consumer not in ('frozen', 'ordered', 'parent'):
            raise ValueError('consumer must be frozen, ordered or parent')
        if self.terminal_route and self.consumer != 'frozen':
            raise ValueError('terminal_route is the tested frozen SELL composition')
        if self.terminal_history and (self.consumer == 'parent' or self.history_hypotheses is None):
            raise ValueError('terminal_history needs a SELL snapshot and explicit scenario hypotheses')
        if not 0 <= self.reserve_seconds < self.budget_seconds <= 1:
            raise ValueError('invalid action deadline')


class TitanAgent:
    """Construction is lazy so the guard includes the chosen parent's cold start.

    Ordered mode consumes its existing ledger, unit projection and seed funding
    internally. Frozen mode owns its frozen ledger and applies ALDER/JUNIPER once
    after SELL, matching the shipped seed_main order. Ledgers never compose twice.
    """
    def __init__(self, features=None):
        self.features = features or Features()
        self.ready = False
        self.diagnostics = {}
        self.selected = None
        self.history = None
        self.post = None
        self._completed_route = None

    def _initialize(self):
        f = self.features
        self.funding_module = load('_titan_funding', HERE/'reference/titan-current/seed_funding.py', cache=True)
        selector = self.funding_module.select_seed_queue if f.funding else None
        if f.consumer == 'ordered':
            from integrated_selected import IntegratedSelectedAgent
            self.consumer = IntegratedSelectedAgent(seed=f.seed, committed=f.committed,
                                                   sell=True, seed_queue_selector=selector)
            self.production = self.consumer.production
            self.controller = self.consumer.controller
        else:
            from frozen_selected import FrozenSelected
            self.consumer = FrozenSelected()
            self.consumer.capture_post_units = f.terminal_history
            self.controller = self.consumer.controller
            self.production = self.controller
            if f.terminal_route:
                module = load('_titan_terminal_composition', HERE/'reference/titan-current/terminal_composition.py')
                self.production = module.TerminalOwner(self.controller)
                self.consumer.controller = self.production
            budget = load('_titan_seed_budget', HERE/'reference/integrated-selected/alder/seed_budget.py', cache=True)
            self.seed_budget = budget.SeedBudget(self.controller.R)
        if f.terminal_history and self.history is None:
            from terminal_history_join import TerminalHistoryJoin
            self.history = TerminalHistoryJoin(hypotheses=f.history_hypotheses,
                                               tie_break=f.terminal_tie_break)
        if self._completed_route is not None:
            self.controller.cur = self._completed_route
        self.ready = True

    def _seed_selected(self, obs, cfg, selected):
        if not self.features.seed or not any(o and o[0] == 'BUY_SEED' for o in selected['market']):
            return selected
        from scheduler import post_units, m
        snapshot = getattr(self.consumer, 'selected_post_units', None)
        farm, private = snapshot if snapshot is not None else post_units(obs, selected, cfg)
        proposed = self.seed_budget.apply(selected, private['seeds'], int(obs['step']),
                                         self.controller.cur, int(cfg.get('maxMarketOrdersPerTurn', 10)))
        edits = [i for i, (a, b) in enumerate(zip(selected['market'], proposed['market'])) if a != b]
        dependent = any(o and o[0] in ('HIRE', 'BUY_LAND', 'BUY_PRODUCT', 'BUY_ANIMAL')
                        for i in edits for o in selected['market'][i+1:])
        if not dependent:
            return proposed
        if not self.features.funding:
            return selected
        post = deepcopy(obs)
        post['farms'][int(obs['player'])] = farm
        post['private'] = private
        result, report = self.funding_module.select_seed_queue(
            m, post, deepcopy(selected), deepcopy(proposed), deepcopy(cfg))
        self.diagnostics['seed_funding'] = report
        return result

    def transform_selected(self, obs, cfg, selected):
        """Dispatch an already-selected action; never calls a producer."""
        if self.features.consumer == 'ordered':
            return self.consumer.transform(obs, cfg, selected, fallback_action=selected)
        result = (self.consumer.transform(obs, cfg, selected)
                  if self.features.consumer == 'frozen' else deepcopy(selected))
        return self._seed_selected(obs, cfg, result)

    def _selected_snapshot(self, obs):
        if self.features.consumer == 'ordered':
            packet = self.consumer.last_packet
            return None if packet is None else packet['post_unit_observation']
        pair = getattr(self.consumer, 'selected_post_units', None)
        if pair is None:return None
        post = deepcopy(obs)
        post['farms'][int(obs['player'])], post['private'] = pair
        return post

    def act(self, observation, configuration=None, *, entry_started=None):
        invoked = time.perf_counter()
        # The canonical entrypoint shares its start clock with this same timer.
        # A supplied future timestamp must never extend the configured budget.
        started = invoked if entry_started is None else min(invoked,float(entry_started))
        cpu_started = time.process_time()
        cfg = dict(configuration or {})
        obs = dict(observation)
        obs['step'] = int(obs['step']) if obs.get('step') is not None else int(obs['day'])*int(cfg.get('turnsPerDay', 24))+int(obs['hour'])
        last = int(cfg.get('episodeSteps', 720))-2
        fallback = (deadline.terminal_liquidation_fallback(obs, cfg) if obs['step'] == last
                    else deadline.legal_pass(obs))
        self.selected = None
        self.post = None
        self.diagnostics = {'consumer': self.features.consumer, 'parent_calls': 0,
                            'entrypoint_prelude_seconds': invoked-started}
        selected_checkpoint = None
        stage = 'cold_start'
        seconds = self.features.budget_seconds-self.features.reserve_seconds-(time.perf_counter()-started)
        if seconds <= 0:
            self.diagnostics.update(status='deadline_fallback',fallback_stage='entrypoint_prelude',
                elapsed_seconds=time.perf_counter()-started,act_cpu_seconds=time.process_time()-cpu_started)
            return fallback
        timer = deadline._DeadlineTimer(seconds)
        try:
            with timer:
                if not self.ready:
                    self._initialize()
                if self.history is not None:
                    stage = 'history_observation'
                    self.history.observe(obs)
                if self.features.consumer == 'ordered':
                    self.consumer.last_packet = None
                else:
                    self.consumer.selected_post_units = None
                stage = 'production'
                if self.features.terminal_route:
                    self.production.configuration = cfg
                self.diagnostics['parent_calls'] = 1
                selected = self.production.act(obs)
                self.selected = deepcopy(selected)
                selected_checkpoint = (self.selected, self.controller.cur)
                fallback = selected_checkpoint[0]
                stage = 'selected_transform'
                output = self.transform_selected(obs, cfg, selected)
                if self.history is not None:
                    self.post = self._selected_snapshot(obs)
                    stage = 'terminal_history'
                    output = self.history.transform(obs,cfg,output,self.post,
                        deadline=started+self.features.budget_seconds-self.features.reserve_seconds)
                    self.history.remember(obs,cfg,output,self.post)
                    self.diagnostics['history'] = self.history.diagnostics
                self._completed_route = selected_checkpoint[1]
                self.diagnostics.update(status='completed', elapsed_seconds=time.perf_counter()-started,
                                        act_cpu_seconds=time.process_time()-cpu_started)
                return output
        except deadline.DeadlineExceeded as error:
            if error is not timer.expired:
                raise
            # Cancellation can interrupt a state mutation. Reconstruct next turn
            # from observed state rather than reuse partially updated ledgers.
            self.ready = False
            # Retain only the route paired with a complete selected fallback.
            # A producer interrupted before returning cannot commit its choice.
            if selected_checkpoint is not None and fallback is selected_checkpoint[0]:
                self._completed_route = selected_checkpoint[1]
            if self.history is not None:
                if stage in ('cold_start','history_observation'):
                    self.history = None
                else:
                    # Bind only the exact returned fallback and a completed
                    # current unit snapshot. Reserve time covers this copy.
                    self.history.remember(obs,cfg,fallback,self._selected_snapshot(obs))
            output = deepcopy(fallback)
            self.diagnostics.update(status='deadline_fallback', fallback_stage=stage,
                                    elapsed_seconds=time.perf_counter()-started,
                                    act_cpu_seconds=time.process_time()-cpu_started)
            return output

    __call__ = act
