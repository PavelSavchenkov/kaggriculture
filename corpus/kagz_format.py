"""Pure .kagz container read/write. No kaggle_environments dependency.

A Kaggriculture episode is fully determined by (configuration, seed, action
stream): replaying the recorded actions through the pinned engine regenerates
every state bit-exactly. So the canonical stored form is the action stream, and
the engine is the decompressor.

Actions are stored as generic symbol/int element lists rather than the
normalized (op, arg, count) triples used by fast_game_engine's trace format.
That costs a few bytes and buys byte-lossless reconstruction of the original
Kaggle JSON: the trace triples cannot distinguish ["PICKUP","COW"] from
["PICKUP","COW",1], nor preserve a stray argument such as ["HIRE",70], both of
which occur in real replays.
"""
import json
import struct
import zlib

MAGIC = b"KAGZ"
VERSION = 1

# Order matters: it is the on-disk config field order.
CONFIG_FIELDS = [
    ("episodeSteps", "u32"), ("boardSize", "u32"), ("startingMoney", "u32"),
    ("maxMarketOrdersPerTurn", "u32"), ("turnsPerDay", "u32"),
    ("shedCapacity", "u32"), ("weedSpawnChance", "f64"),
    ("townShopUnlockInterval", "u32"), ("townShopSellInterval", "u32"),
    ("townCenterSellInterval", "u32"), ("farmHandCostMult", "u32"),
    ("actTimeout", "u32"), ("runTimeout", "u32"),
]

ELEM_INT = 0xFE          # element tag: a varint integer follows
ELEM_NULL = 0xFF         # element tag: a JSON null
MAX_SYMBOLS = 0xFE       # tags 0x00..0xFD index the symbol table


class Writer:
    def __init__(self):
        self.buf = bytearray()

    def u8(self, v):
        self.buf.append(v & 0xFF)

    def u32(self, v):
        self.buf += struct.pack("<I", v & 0xFFFFFFFF)

    def u64(self, v):
        self.buf += struct.pack("<Q", v & 0xFFFFFFFFFFFFFFFF)

    def f64(self, v):
        self.buf += struct.pack("<d", v)

    def varint(self, v):
        # zigzag so negative quantities (they occur in adversarial orders) fit
        v = (v << 1) ^ (v >> 63) if v < 0 else (v << 1)
        while True:
            b = v & 0x7F
            v >>= 7
            if v:
                self.buf.append(b | 0x80)
            else:
                self.buf.append(b)
                break

    def blob(self, b):
        self.varint(len(b))
        self.buf += b

    def text(self, s):
        self.blob(s.encode("utf-8"))


class Reader:
    def __init__(self, buf, pos=0):
        self.buf = buf
        self.pos = pos

    def u8(self):
        v = self.buf[self.pos]
        self.pos += 1
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.buf, self.pos)[0]
        self.pos += 4
        return v

    def u64(self):
        v = struct.unpack_from("<Q", self.buf, self.pos)[0]
        self.pos += 8
        return v

    def f64(self):
        v = struct.unpack_from("<d", self.buf, self.pos)[0]
        self.pos += 8
        return v

    def varint(self):
        shift = 0
        result = 0
        while True:
            b = self.buf[self.pos]
            self.pos += 1
            result |= (b & 0x7F) << shift
            if not (b & 0x80):
                break
            shift += 7
        return (result >> 1) ^ -(result & 1)

    def blob(self):
        n = self.varint()
        v = bytes(self.buf[self.pos:self.pos + n])
        self.pos += n
        return v

    def text(self):
        return self.blob().decode("utf-8")


def _encode_actions(w, actions, symbols):
    """actions: [step][seat] -> {'farmer': [...], 'hands': [[...]], 'market': [[...]]}"""
    index = {s: i for i, s in enumerate(symbols)}

    def element_list(lst):
        w.varint(len(lst))
        for el in lst:
            if el is None:
                # Real replays contain nulls inside actions. The engine tolerates
                # them - enc_order maps a null item to 255 and the order is simply
                # rejected - so the codec has to round-trip them rather than
                # refuse the episode.
                w.u8(ELEM_NULL)
            elif isinstance(el, str):
                w.u8(index[el])
            elif isinstance(el, bool):
                w.u8(ELEM_INT)
                w.varint(int(el))
            elif isinstance(el, int):
                w.u8(ELEM_INT)
                w.varint(el)
            else:
                raise ValueError(f"unsupported action element {el!r}")

    extras = []
    for t, step in enumerate(actions):
        for seat_index, seat in enumerate(step):
            unknown = {k: v for k, v in seat.items()
                       if k not in ("farmer", "hands", "market")}
            if unknown:
                extras.append((t, seat_index, json.dumps(unknown, sort_keys=True)))
            present = (("farmer" in seat) << 0) | (("hands" in seat) << 1) | (("market" in seat) << 2)
            w.u8(present)
            if "farmer" in seat:
                element_list(seat["farmer"])
            hands = seat.get("hands") or []
            w.varint(len(hands))
            for h in hands:
                element_list(h)
            market = seat.get("market") or []
            w.varint(len(market))
            for m in market:
                element_list(m)

    # Some agents attach extra keys to their action - carbonapi echoes its whole
    # input observation back as `policy_observation`. Those keys are not game
    # state and the engine ignores them, but they are in the recorded file, so
    # they are preserved verbatim rather than special-cased per agent.
    w.varint(len(extras))
    for t, seat_index, blob in extras:
        w.varint(t)
        w.varint(seat_index)
        w.text(blob)


def _encode_overage(w, overage):
    """Per-(step, seat) remainingOverageTime. Not game state - the engine never
    reads it - but it is part of the recorded observation and it is the only
    direct measure of how much compute each agent spent per turn.

    Stored as a table of distinct JSON scalars plus one index per slot. The
    values are raw wall-clock doubles, not microsecond multiples, so quantizing
    them loses a low bit and breaks exact reconstruction; a table keeps them
    verbatim (including int-vs-float) and stays small because agents hold the
    same value for long runs. Index 0 means absent.
    """
    w.u8(1 if overage is not None else 0)
    if overage is None:
        return
    table, index = [], {}
    for row in overage:
        for v in row:
            if v is None:
                continue
            key = repr(v)
            if key not in index:
                index[key] = len(table) + 1
                table.append(json.dumps(v))
    w.varint(len(table))
    for entry in table:
        w.text(entry)
    for row in overage:
        for v in row:
            w.varint(0 if v is None else index[repr(v)])


def _decode_overage(r, n_steps, n_seats):
    if not r.u8():
        return None
    table = [None] + [json.loads(r.text()) for _ in range(r.varint())]
    return [[table[r.varint()] for _ in range(n_seats)] for _ in range(n_steps)]


def _decode_actions(r, n_steps, n_seats, symbols):
    def element_list():
        n = r.varint()
        out = []
        for _ in range(n):
            tag = r.u8()
            if tag == ELEM_NULL:
                out.append(None)
            else:
                out.append(r.varint() if tag == ELEM_INT else symbols[tag])
        return out

    actions = []
    for _ in range(n_steps):
        step = []
        for _ in range(n_seats):
            present = r.u8()
            seat = {}
            if present & 1:
                seat["farmer"] = element_list()
            hands = [element_list() for _ in range(r.varint())]
            if present & 2:
                seat["hands"] = hands
            market = [element_list() for _ in range(r.varint())]
            if present & 4:
                seat["market"] = market
            step.append(seat)
        actions.append(step)
    for _ in range(r.varint()):
        t = r.varint()
        seat_index = r.varint()
        actions[t][seat_index].update(json.loads(r.text()))
    return actions


def collect_symbols(actions):
    seen = {}
    for step in actions:
        for seat in step:
            lists = [seat.get("farmer") or []] + list(seat.get("hands") or []) + list(seat.get("market") or [])
            for lst in lists:
                for el in lst:
                    if isinstance(el, str) and el not in seen:
                        seen[el] = len(seen)
    if len(seen) > MAX_SYMBOLS:
        raise ValueError(f"{len(seen)} distinct action symbols exceeds {MAX_SYMBOLS}")
    return list(seen)


def write(path, *, episode_id, seed, config, teams, statuses, rewards,
          actions, engine_version, engine_sha256, anchors, chain_hash,
          submission_ids=None, extra=None, overage=None):
    """anchors: [(step_index, parity_hash)]. chain_hash: FNV over all 720 hashes."""
    symbols = collect_symbols(actions)
    body = Writer()
    _encode_actions(body, actions, symbols)
    _encode_overage(body, overage)
    packed = zlib.compress(bytes(body.buf), 9)

    w = Writer()
    w.buf += MAGIC
    w.u8(VERSION)
    w.u8(0)                                   # flags
    w.text(engine_version)
    w.buf += bytes.fromhex(engine_sha256)
    w.u64(episode_id)
    w.u64(seed)
    w.u32(len(actions))
    w.u8(len(actions[0]) if actions else 0)   # seats
    for name, kind in CONFIG_FIELDS:
        v = config.get(name)
        if kind == "f64":
            w.f64(0.0 if v is None else float(v))
        else:
            w.u32(0 if v is None else int(v))
    for t in teams:
        w.text(t)
    for s in statuses:
        w.text(s)
    for v in rewards:
        w.f64(float(v))
    subs = submission_ids or []
    w.varint(len(subs))
    for s in subs:
        w.u64(int(s))
    w.text(extra or "")
    w.varint(len(symbols))
    for s in symbols:
        w.text(s)
    w.varint(len(anchors))
    for step, h in anchors:
        w.u32(step)
        w.u64(h)
    w.u64(chain_hash)
    w.blob(packed)
    with open(path, "wb") as f:
        f.write(bytes(w.buf))
    return len(w.buf)


def read(path):
    with open(path, "rb") as f:
        raw = f.read()
    if raw[:4] != MAGIC:
        raise ValueError(f"{path}: not a .kagz file")
    r = Reader(raw, 4)
    version = r.u8()
    if version != VERSION:
        raise ValueError(f"{path}: unsupported .kagz version {version}")
    r.u8()                                    # flags
    engine_version = r.text()
    engine_sha256 = bytes(r.buf[r.pos:r.pos + 32]).hex()
    r.pos += 32
    episode_id = r.u64()
    seed = r.u64()
    n_steps = r.u32()
    n_seats = r.u8()
    config = {}
    for name, kind in CONFIG_FIELDS:
        config[name] = r.f64() if kind == "f64" else r.u32()
    teams = [r.text(), r.text()]
    statuses = [r.text(), r.text()]
    rewards = [r.f64(), r.f64()]
    submission_ids = [r.u64() for _ in range(r.varint())]
    extra = r.text()
    symbols = [r.text() for _ in range(r.varint())]
    anchors = [(r.u32(), r.u64()) for _ in range(r.varint())]
    chain_hash = r.u64()
    body = zlib.decompress(r.blob())
    br = Reader(body)
    actions = _decode_actions(br, n_steps, n_seats, symbols)
    overage = _decode_overage(br, n_steps, n_seats)
    return {
        "engine_version": engine_version, "engine_sha256": engine_sha256,
        "episode_id": episode_id, "seed": seed, "n_steps": n_steps,
        "config": config, "teams": teams, "statuses": statuses,
        "rewards": rewards, "submission_ids": submission_ids, "extra": extra,
        "anchors": anchors, "chain_hash": chain_hash, "actions": actions,
        "overage": overage,
    }
