# -*- coding: utf-8 -*-
"""Radiografía de un agente de Kaggriculture a partir de sus replays PÚBLICOS.

Todo lo que hay aquí se puede calcular con lo que Kaggle publica de cada
episodio (`https://www.kaggleusercontent.com/episodes/<id>.json`): las
observaciones de los dos asientos (incluido `private.shed`, que Kaggle
guarda en el replay) y la acción que cada agente devolvió en cada paso.
No se re-simula nada y no hace falta ningún dato privado.

Este fichero es autocontenido a propósito (solo librería estándar; matplotlib
únicamente para las figuras): se incrusta tal cual en el notebook publicado,
de modo que el código que se enseña es exactamente el que produjo las cifras.

Uso mínimo:

    raw = cargar_replay(106676748)                 # o ruta local .json / .json.xz
    yo  = asiento_por_nombre(raw, "NombreDelEquipo")
    p   = analiza_partida(raw, yo)
    res = resumen([p, ...])
"""
from __future__ import annotations

import gzip
import hashlib
import json
import lzma
import statistics
import time
import urllib.request
from collections import Counter, defaultdict
from pathlib import Path

EPISODE_URL = "https://www.kaggleusercontent.com/episodes/{id}.json"
URL_EPISODIO = "https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-{id}"
UA = {"User-Agent": "Mozilla/5.0", "Accept-Encoding": "gzip"}
TURNOS_DIA = 24
HORAS_HUELLA = (20, 44, 68, 92)      # hora 20 de los días 0-3: todos fuera de casa, las posiciones distinguen
PASOS_TIENDA = (72, 144, 216)
PRODUCTOS = ("WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER")
ANIMALES = ("GOOSE", "COW", "SHEEP")


# --------------------------------------------------------------------------
# carga
# --------------------------------------------------------------------------

def cargar_replay(eid_o_ruta, cache_dir=None, pausa: float = 0.0) -> dict:
    """Replay como dict. Acepta un id de episodio (lo baja de Kaggle, con gzip) o
    una ruta local `.json` / `.json.xz`. Si se da `cache_dir`, guarda y reutiliza."""
    p = Path(str(eid_o_ruta))
    if p.exists():
        datos = p.read_bytes()
        if p.suffix == ".xz":
            datos = lzma.decompress(datos)
        return json.loads(datos)
    eid = int(eid_o_ruta)
    if cache_dir:
        for c in (Path(cache_dir) / f"{eid}.json.xz", Path(cache_dir) / f"{eid}.json"):
            if c.exists():
                return cargar_replay(c)
    for intento in range(4):
        try:
            req = urllib.request.Request(EPISODE_URL.format(id=eid), headers=UA)
            with urllib.request.urlopen(req, timeout=90) as r:
                datos = r.read()
                if (r.headers.get("Content-Encoding") or "").lower() == "gzip":
                    datos = gzip.decompress(datos)
            break
        except Exception as e:      # noqa: BLE001
            cod = getattr(e, "code", None)
            time.sleep(60 * (intento + 1) if cod == 429 else 5 * (intento + 1))
    else:
        raise RuntimeError(f"no pude bajar el episodio {eid}")
    if cache_dir:
        Path(cache_dir).mkdir(parents=True, exist_ok=True)
        tmp = Path(cache_dir) / f"{eid}.parcial"
        tmp.write_bytes(lzma.compress(datos, preset=1))
        tmp.replace(Path(cache_dir) / f"{eid}.json.xz")
    if pausa:
        time.sleep(pausa)
    return json.loads(datos)


def asiento_por_nombre(raw: dict, nombre: str) -> int:
    """Índice (0/1) del equipo `nombre` en el replay; -1 si no está."""
    nombres = (raw.get("info") or {}).get("TeamNames") or []
    for i, n in enumerate(nombres):
        if n == nombre:
            return i
    return -1


# --------------------------------------------------------------------------
# una partida
# --------------------------------------------------------------------------

def _norm(a) -> str:
    return json.dumps(a, sort_keys=True, separators=(",", ":")) if a else ""


def _h(a) -> str:
    return hashlib.md5(_norm(a).encode()).hexdigest()[:10]


def huella(granja: dict) -> str | None:
    """Posición del granjero y de las manos: "x,y|x,y;x,y"."""
    try:
        f = granja.get("farmer")
        hs = granja.get("hands") or []
        return "%d,%d|%s" % (int(f[0]), int(f[1]),
                             ";".join("%d,%d" % (int(h[0]), int(h[1])) for h in hs))
    except Exception:      # noqa: BLE001
        return None


def _obs(steps, t, s) -> dict:
    try:
        return (steps[t][s] or {}).get("observation") or {}
    except (IndexError, TypeError):
        return {}


def _accion(steps, t, s) -> dict:
    """Acción que el asiento `s` devolvió ante la observación del paso `t`
    (en el replay de Kaggle va guardada en `steps[t+1]`)."""
    try:
        return (steps[t + 1][s] or {}).get("action") or {}
    except (IndexError, TypeError):
        return {}


def analiza_partida(raw: dict, yo: int, eid=None) -> dict:
    """Todo lo que se lee de un replay sin re-simular, desde el asiento `yo`.
    `eid` es el número de episodio de Kaggle (el `id` interno del replay es un UUID)."""
    steps = raw["steps"]
    riv = 1 - yo
    n = len(steps)
    info = raw.get("info") or {}
    dias: dict = defaultdict(lambda: {s: {
        "ped": Counter(), "eje": Counter(), "val": Counter(), "compras": Counter(),
        "gasto_prod": Counter(), "semillas": Counter(), "animales_compra": Counter(),
        "manos": None, "sembrado": Counter(), "animales": Counter(), "inv": Counter(),
        "pastos": 0, "corrales": 0, "dinero": None, "cuadrantes": None,
        "verbos": Counter(), "ordenes": 0, "contrata": 0, "tierra": 0} for s in (0, 1)})
    primer_distinto = None
    iguales = 0
    apertura = []                 # órdenes de mercado del asiento `yo` en t0..t5
    hashes = []                   # una huella por paso de la acción de `yo`
    primera_venta = None
    contrataciones = []           # (paso, cuántas)
    tierras = []                  # pasos con BUY_LAND
    animales_compra = []          # (paso, tipo, q)
    final = {"drop": 0, "sell_ordenes": 0, "sell_uds": 0}
    for t in range(n - 1):
        dia = t // TURNOS_DIA
        acts = {}
        for s in (0, 1):
            o = _obs(steps, t, s)
            if not o:
                continue
            pr = o.get("private") or {}
            shed = dict(pr.get("shed") or {})
            precios = (o.get("market") or {}).get("prices") or {}
            acc = _accion(steps, t, s)
            acts[s] = acc
            d = dias[dia][s]
            manos_acc = list(acc.get("hands") or [])
            for a in [acc.get("farmer")] + manos_acc:
                if isinstance(a, list) and a:
                    d["verbos"][str(a[0])] += 1
            ords = acc.get("market") or []
            d["ordenes"] += len(ords)
            for ordn in ords:
                if not ordn:
                    continue
                tipo = str(ordn[0])
                if tipo == "SELL" and len(ordn) >= 3:
                    item = str(ordn[1])
                    q = int(ordn[2] or 0)
                    hay = int(shed.get(item, 0) or 0)
                    qe = max(0, min(q, hay))
                    shed[item] = hay - qe
                    d["ped"][item] += q
                    d["eje"][item] += qe
                    p = precios.get(item)
                    if p is not None:
                        d["val"][item] += qe * int(p)
                    if s == yo:
                        if primera_venta is None and qe > 0:
                            primera_venta = {"paso": t, "dia": dia, "item": item, "uds": qe}
                        if t >= 700:
                            final["sell_ordenes"] += 1
                            final["sell_uds"] += qe
                elif tipo.startswith("BUY") and len(ordn) >= 3:
                    d["compras"][f"{tipo}:{ordn[1]}"] += int(ordn[2] or 0)
                    if tipo == "BUY_PRODUCT" and precios.get(str(ordn[1])) is not None:
                        d["gasto_prod"][str(ordn[1])] += int(ordn[2] or 0) * int(precios[str(ordn[1])])
                    if tipo == "BUY_SEED":
                        d["semillas"][str(ordn[1])] += int(ordn[2] or 0)
                    if tipo == "BUY_ANIMAL":
                        d["animales_compra"][str(ordn[1])] += int(ordn[2] or 0)
                        if s == yo:
                            animales_compra.append((t, str(ordn[1]), int(ordn[2] or 0)))
                else:
                    d["compras"][tipo] += 1
                    if tipo == "HIRE":
                        d["contrata"] += 1
                    if tipo == "BUY_LAND":
                        d["tierra"] += 1
                        if s == yo:
                            tierras.append(t)
            if s == yo:
                nh = sum(1 for x in ords if x and str(x[0]) == "HIRE")
                if nh:
                    contrataciones.append((t, nh))
                if t <= 5:
                    apertura.append(ords)
                hashes.append(_h(acc))
                if t >= 700:
                    final["drop"] += sum(1 for a in [acc.get("farmer")] + manos_acc
                                         if isinstance(a, list) and a and a[0] == "DROP")
            farms = o.get("farms") or []
            if len(farms) == 2:
                f = farms[s]
                d["dinero"] = f.get("money")
                d["manos"] = len(f.get("hands") or [])
                d["cuadrantes"] = len(f.get("unlocked_quadrants") or [])
                semb = Counter()
                anim = Counter()
                pastos = corrales = 0
                for fila in f.get("tiles") or []:
                    for tile in fila or []:
                        if isinstance(tile, dict):
                            if tile.get("crop"):
                                semb[str(tile["crop"])] += 1
                            if tile.get("animal"):
                                anim[str(tile["animal"])] += 1
                            k = tile.get("kind")
                            pastos += k == "PASTURE"
                            corrales += k == "COOP"
                d["sembrado"], d["animales"] = semb, anim
                d["pastos"], d["corrales"] = pastos, corrales
                inv = Counter({k: int(v) for k, v in (pr.get("shed") or {}).items() if v})
                for bolsa in pr.get("inventories") or []:
                    for k, v in (bolsa or {}).items():
                        inv[k] += int(v)
                d["inv"] = inv
        if len(acts) == 2:
            if _norm(acts[0]) == _norm(acts[1]):
                iguales += 1
            elif primer_distinto is None:
                primer_distinto = t
    # tiendas (las ve igual cualquier asiento)
    tiendas = {}
    for t in PASOS_TIENDA:
        if t < n:
            tiendas[str(t)] = list((_obs(steps, t, 0).get("town") or {}).get("unlocked_shops") or [])
    # huellas de tablero de los dos asientos a la hora 20 de los días 0-3
    hu_yo, hu_riv = {}, {}
    for t in HORAS_HUELLA:
        farms = _obs(steps, t, 0).get("farms") or []
        hu_yo[str(t)] = huella(farms[yo]) if len(farms) == 2 else None
        hu_riv[str(t)] = huella(farms[riv]) if len(farms) == 2 else None
    if hu_yo.get("20") and hu_yo["20"] == hu_riv.get("20"):
        clase = "gemelo" if all(hu_yo[k] and hu_yo[k] == hu_riv[k] for k in hu_yo) else "misma_apertura"
    else:
        clase = "distinto"
    # serie por día
    serie = []
    prev = 0
    for dia in sorted(dias):
        du, dr = dias[dia][yo], dias[dia][riv]
        if du["dinero"] is None or dr["dinero"] is None:
            continue
        hueco = du["dinero"] - dr["dinero"]
        fila = {"dia": dia, "yo": du["dinero"], "riv": dr["dinero"], "hueco": hueco,
                "delta": hueco - prev}
        for k in ("manos", "cuadrantes", "pastos", "corrales", "ordenes", "contrata", "tierra"):
            fila[k + "_yo"], fila[k + "_riv"] = du[k], dr[k]
        for k in ("sembrado", "animales", "eje", "ped", "val", "compras", "gasto_prod", "inv",
                  "verbos", "semillas", "animales_compra"):
            fila[k + "_yo"], fila[k + "_riv"] = dict(du[k]), dict(dr[k])
        serie.append(fila)
        prev = hueco
    peor = min(serie, key=lambda x: x["delta"]) if serie else None
    mejor = max(serie, key=lambda x: x["delta"]) if serie else None
    tot_yo, tot_riv, ped_yo = Counter(), Counter(), Counter()
    for x in serie:
        tot_yo.update(x["eje_yo"])
        tot_riv.update(x["eje_riv"])
        ped_yo.update(x["ped_yo"])
    rewards = [(steps[-1][s] or {}).get("reward") for s in (0, 1)]
    statuses = raw.get("statuses") or [None, None]
    r_yo, r_riv = rewards[yo], rewards[riv]
    if r_yo is None or r_riv is None:
        resultado = "?"
    else:
        resultado = "V" if r_yo > r_riv else ("D" if r_yo < r_riv else "E")
    return {"eid": eid if eid is not None else raw.get("id"), "seed": info.get("seed"), "nombres": info.get("TeamNames"),
            "yo": yo, "rival": (info.get("TeamNames") or [None, None])[riv],
            "reward_yo": r_yo, "reward_riv": r_riv, "resultado": resultado,
            "margen": (r_yo - r_riv) if (r_yo is not None and r_riv is not None) else None,
            "status_yo": statuses[yo] if len(statuses) == 2 else None,
            "status_riv": statuses[riv] if len(statuses) == 2 else None,
            "pasos": n, "tiendas": tiendas, "huella_yo": hu_yo, "huella_riv": hu_riv, "clase": clase,
            "primer_paso_distinto": primer_distinto, "pasos_iguales": iguales,
            "apertura": apertura, "hashes": hashes, "primera_venta": primera_venta,
            "contrataciones": contrataciones, "tierras": tierras, "animales_compra": animales_compra,
            "final": final, "serie": serie,
            "peor_dia": peor["dia"] if peor else None, "peor_delta": peor["delta"] if peor else None,
            "mejor_dia": mejor["dia"] if mejor else None, "mejor_delta": mejor["delta"] if mejor else None,
            "ventas_yo": dict(tot_yo), "pedido_yo": dict(ped_yo), "ventas_riv": dict(tot_riv),
            "valor_yo": sum(sum(x["val_yo"].values()) for x in serie),
            "valor_riv": sum(sum(x["val_riv"].values()) for x in serie)}


# --------------------------------------------------------------------------
# agregados sobre muchas partidas
# --------------------------------------------------------------------------

def _media(xs):
    xs = [x for x in xs if x is not None]
    return statistics.fmean(xs) if xs else None


def _mediana(xs):
    xs = [x for x in xs if x is not None]
    return statistics.median(xs) if xs else None


def resumen(partidas: list) -> dict:
    """Agregados de una lista de `analiza_partida` (todas del mismo agente)."""
    P = [p for p in partidas if p.get("resultado") in ("V", "D", "E")]
    n = len(P)
    V = sum(p["resultado"] == "V" for p in P)
    D = sum(p["resultado"] == "D" for p in P)
    E = n - V - D
    out: dict = {"n": n, "V": V, "D": D, "E": E, "tasa": (V / n) if n else None,
                 "margen_medio": _media([p["margen"] for p in P]),
                 "margen_mediano": _mediana([p["margen"] for p in P]),
                 "reward_medio": _media([p["reward_yo"] for p in P]),
                 "status": dict(Counter(str(p["status_yo"]) for p in P))}
    # por clase de rival y por primera tienda
    for clave, f in (("por_clase", lambda p: p["clase"]),
                     ("por_tienda72", lambda p: (p["tiendas"].get("72") or ["?"])[0]),
                     ("por_rival", lambda p: p["rival"])):
        g: dict = {}
        for p in P:
            k = f(p)
            x = g.setdefault(k, {"n": 0, "V": 0, "D": 0, "E": 0, "margen": [], "eids": []})
            x["n"] += 1
            x[p["resultado"]] += 1
            x["margen"].append(p["margen"])
            x["eids"].append(p["eid"])
        for x in g.values():
            x["margen_medio"] = _media(x["margen"])
            del x["margen"]
        out[clave] = dict(sorted(g.items(), key=lambda kv: -kv[1]["n"]))
    # apertura: variantes de las órdenes de mercado t0..t5
    for t in range(6):
        c = Counter(_norm(p["apertura"][t]) if len(p["apertura"]) > t else "" for p in P)
        out[f"apertura_t{t}"] = [(k or "[]", v) for k, v in c.most_common(6)]
    out["contrataciones"] = Counter()
    out["contrataciones_pasos"] = Counter()
    for p in P:
        for t, k in p["contrataciones"]:
            out["contrataciones"][t // TURNOS_DIA] += k
            out["contrataciones_pasos"][t] += k
    # medias POR PARTIDA (las manos se contratan cada día, así que son cientos por partida)
    out["contrataciones_partida"] = sum(out["contrataciones"].values()) / n if n else None
    out["contrataciones"] = {d: round(k / n, 2) for d, k in sorted(out["contrataciones"].items())}
    out["contrataciones_pasos"] = {t: round(k / n, 2) for t, k in sorted(out["contrataciones_pasos"].items())}
    out["tierras"] = {d: round(k / n, 2) for d, k in sorted(Counter(t // TURNOS_DIA for p in P for t in p["tierras"]).items())}
    out["animales_compra"] = {f"d{d} {a}": round(k / n, 2) for (d, a), k in sorted(Counter(
        (t // TURNOS_DIA, a) for p in P for t, a, q in p["animales_compra"] for _ in range(q)).items())}
    out["primera_venta_dia"] = dict(sorted(Counter(p["primera_venta"]["dia"] for p in P
                                                   if p["primera_venta"]).items()))
    out["primera_venta_item"] = dict(Counter(p["primera_venta"]["item"] for p in P if p["primera_venta"]))
    # rigidez: por paso, fracción de partidas que hacen la acción modal
    L = min((len(p["hashes"]) for p in P), default=0)
    rig = []
    for t in range(L):
        c = Counter(p["hashes"][t] for p in P)
        rig.append(c.most_common(1)[0][1] / n)
    out["rigidez_paso"] = rig
    out["rigidez_dia"] = [_media(rig[d * TURNOS_DIA:(d + 1) * TURNOS_DIA]) for d in range(L // TURNOS_DIA)]
    # divergencia entre pares de partidas: primer paso con acción distinta
    div = []
    for i in range(n):
        for j in range(i + 1, n):
            a, b = P[i]["hashes"], P[j]["hashes"]
            m = min(len(a), len(b))
            k = next((t for t in range(m) if a[t] != b[t]), m)
            div.append(k)
    out["divergencia_pares"] = {"n_pares": len(div), "mediana": _mediana(div), "media": _media(div),
                                "min": min(div) if div else None, "max": max(div) if div else None,
                                "hist_dia": dict(sorted(Counter(k // TURNOS_DIA for k in div).items()))}
    out["primer_paso_distinto_rival"] = dict(sorted(Counter(
        f"d{p['primer_paso_distinto'] // TURNOS_DIA}" if p["primer_paso_distinto"] is not None else "nunca"
        for p in P).items()))
    # economía por día (medias), separando victorias y derrotas
    def curva(sel, campo):
        dd: dict = defaultdict(list)
        for p in sel:
            for x in p["serie"]:
                dd[x["dia"]].append(x[campo])
        return [_media(dd[d]) for d in sorted(dd)]
    out["dinero_dia"] = {"todas": curva(P, "yo"), "rival": curva(P, "riv"),
                         "V": curva([p for p in P if p["resultado"] == "V"], "yo"),
                         "D": curva([p for p in P if p["resultado"] == "D"], "yo"),
                         "hueco_V": curva([p for p in P if p["resultado"] == "V"], "hueco"),
                         "hueco_D": curva([p for p in P if p["resultado"] == "D"], "hueco")}
    out["peor_dia"] = dict(sorted(Counter(p["peor_dia"] for p in P if p["resultado"] == "D").items()))
    out["mejor_dia"] = dict(sorted(Counter(p["mejor_dia"] for p in P if p["resultado"] == "V").items()))
    # ventas: media por partida de unidades ejecutadas y pedidas, por producto; por día
    ven, ped, val = Counter(), Counter(), Counter()
    ven_dia: dict = defaultdict(Counter)
    gasto_dia: dict = defaultdict(Counter)
    semillas_dia: dict = defaultdict(Counter)
    for p in P:
        ven.update(p["ventas_yo"])
        ped.update(p["pedido_yo"])
        for x in p["serie"]:
            ven_dia[x["dia"]].update(x["eje_yo"])
            gasto_dia[x["dia"]].update(x["gasto_prod_yo"])
            semillas_dia[x["dia"]].update(x["semillas_yo"])
            val.update(x["val_yo"])
    out["ventas_media"] = {k: ven[k] / n for k in PRODUCTOS if n}
    out["pedido_media"] = {k: ped[k] / n for k in PRODUCTOS if n}
    out["valor_media"] = {k: val[k] / n for k in PRODUCTOS if n}
    out["valor_total_medio"] = _media([p["valor_yo"] for p in P])
    out["ventas_dia"] = {d: {k: c[k] / n for k in PRODUCTOS if c[k]} for d, c in sorted(ven_dia.items())}
    out["gasto_pienso_dia"] = {d: sum(c.values()) / n for d, c in sorted(gasto_dia.items())}
    out["gasto_pienso_medio"] = sum(sum(c.values()) for c in gasto_dia.values()) / n if n else None
    out["semillas_dia"] = {d: dict(c) for d, c in sorted(semillas_dia.items())}
    # estructura de granja por día (medias): manos, cuadrantes, pastos, corrales, animales, sembrado
    est: dict = defaultdict(lambda: defaultdict(list))
    for p in P:
        for x in p["serie"]:
            d = x["dia"]
            for k in ("manos", "cuadrantes", "pastos", "corrales"):
                est[d][k].append(x[k + "_yo"])
            for a in ANIMALES:
                est[d]["anim_" + a].append(x["animales_yo"].get(a, 0))
            for c in ("WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON"):
                est[d]["semb_" + c].append(x["sembrado_yo"].get(c, 0))
    out["granja_dia"] = {d: {k: _media(v) for k, v in kv.items()} for d, kv in sorted(est.items())}
    # verbos (acciones físicas) totales por partida
    verbos = Counter()
    for p in P:
        for x in p["serie"]:
            verbos.update(x["verbos_yo"])
    out["verbos_media"] = {k: v / n for k, v in verbos.most_common()} if n else {}
    out["ordenes_media"] = _media([sum(x["ordenes_yo"] for x in p["serie"]) for p in P])
    out["final"] = {k: _media([p["final"][k] for p in P]) for k in ("drop", "sell_ordenes", "sell_uds")}
    out["tiendas72"] = dict(Counter((p["tiendas"].get("72") or ["?"])[0] for p in P).most_common())
    # gemelos: cuántos rivales distintos comparten las cuatro huellas con él
    out["gemelos"] = sorted({p["rival"] for p in P if p["clase"] == "gemelo"})
    out["misma_apertura"] = sorted({p["rival"] for p in P if p["clase"] == "misma_apertura"})
    # mayores derrotas / victorias
    out["mayores_derrotas"] = sorted([p for p in P if p["resultado"] == "D"], key=lambda p: p["margen"])[:8]
    out["mayores_victorias"] = sorted([p for p in P if p["resultado"] == "V"], key=lambda p: -p["margen"])[:8]
    for k in ("mayores_derrotas", "mayores_victorias"):
        out[k] = [{"eid": p["eid"], "rival": p["rival"], "margen": p["margen"], "reward_yo": p["reward_yo"],
                   "reward_riv": p["reward_riv"], "peor_dia": p["peor_dia"], "clase": p["clase"],
                   "tienda72": (p["tiendas"].get("72") or ["?"])[0], "seed": p["seed"]} for p in out[k]]
    return out


# --------------------------------------------------------------------------
# tablas markdown
# --------------------------------------------------------------------------

def _f(x, dec=0):
    """Número en formato español: miles con punto, decimales con coma."""
    if x is None:
        return "-"
    if isinstance(x, float):
        return f"{x:,.{dec}f}".replace(",", "@").replace(".", ",").replace("@", ".")
    return f"{x:,}".replace(",", ".")


def tabla_marcador(res: dict) -> str:
    L = ["| partidas | V | D | E | tasa | margen medio | margen mediano | reward medio | estados |",
         "|---:|---:|---:|---:|---:|---:|---:|---:|---|",
         f"| {res['n']} | {res['V']} | {res['D']} | {res['E']} | {_f(res['tasa'], 3)} | "
         f"{_f(res['margen_medio'])} | {_f(res['margen_mediano'])} | {_f(res['reward_medio'])} | "
         f"{', '.join(f'{k} {v}' for k, v in res['status'].items())} |"]
    return "\n".join(L)


def tabla_grupos(g: dict, titulo: str, enlace=False) -> str:
    L = [f"| {titulo} | n | V | D | E | tasa | margen medio |", "|---|---:|---:|---:|---:|---:|---:|"]
    for k, x in g.items():
        L.append(f"| {k} | {x['n']} | {x['V']} | {x['D']} | {x['E']} | "
                 f"{_f(x['V'] / x['n'], 3)} | {_f(x['margen_medio'])} |")
    return "\n".join(L)


def tabla_apertura(res: dict) -> str:
    L = ["| paso | órdenes de mercado (variante) | partidas |", "|---:|---|---:|"]
    for t in range(6):
        for k, v in res[f"apertura_t{t}"]:
            L.append(f"| t{t} | `{k}` | {v} |")
    return "\n".join(L)


def tabla_ventas(res: dict) -> str:
    L = ["| producto | uds ejecutadas / partida | uds pedidas / partida | valor / partida |",
         "|---|---:|---:|---:|"]
    for k in PRODUCTOS:
        if res["ventas_media"].get(k) or res["pedido_media"].get(k):
            L.append(f"| {k} | {_f(res['ventas_media'][k], 1)} | {_f(res['pedido_media'][k], 1)} | "
                     f"{_f(res['valor_media'][k])} |")
    return "\n".join(L)


def tabla_extremos(filas: list, titulo: str) -> str:
    L = [f"| {titulo} | rival | clase | tienda t72 | margen | día peor/mejor | enlace |",
         "|---|---|---|---|---:|---:|---|"]
    for p in filas:
        L.append(f"| {p['eid']} | {p['rival']} | {p['clase']} | {p['tienda72']} | {_f(p['margen'])} | "
                 f"{p['peor_dia']} | [replay]({URL_EPISODIO.format(id=p['eid'])}) |")
    return "\n".join(L)


def tabla_granja(res: dict, dias=(0, 1, 2, 3, 5, 8, 12, 16, 20, 24, 29)) -> str:
    L = ["| día | manos | cuadr. | pastos | corrales | vacas | ovejas | gansos | trigo | zanah. | tomate | fresa | melón |",
         "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
    g = res["granja_dia"]
    for d in dias:
        x = g.get(d)
        if not x:
            continue
        L.append(f"| {d} | " + " | ".join(_f(x.get(k), 1) for k in (
            "manos", "cuadrantes", "pastos", "corrales", "anim_COW", "anim_SHEEP", "anim_GOOSE",
            "semb_WHEAT", "semb_CARROT", "semb_TOMATO", "semb_STRAWBERRY", "semb_MELON")) + " |")
    return "\n".join(L)


# --------------------------------------------------------------------------
# figuras (matplotlib)
# --------------------------------------------------------------------------

def figuras(res: dict, out_dir, prefijo: str = "xray", rating: list | None = None) -> dict:
    """Guarda PNGs y devuelve {nombre: ruta}. `rating` = [(fecha_iso, score)]."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    rutas = {}

    fig, ax = plt.subplots(figsize=(8, 4))
    dd = res["dinero_dia"]
    for k, lab, st in (("V", "él, cuando gana", "-"), ("D", "él, cuando pierde", "--"),
                       ("rival", "sus rivales (media)", ":")):
        if dd.get(k):
            ax.plot(range(len(dd[k])), dd[k], st, label=lab)
    ax.set_xlabel("día"); ax.set_ylabel("dinero al cierre del día"); ax.legend(); ax.grid(alpha=.3)
    ax.set_title("Curva de dinero por día")
    fig.tight_layout(); r = out_dir / f"{prefijo}_dinero.png"; fig.savefig(r, dpi=110); plt.close(fig)
    rutas["dinero"] = r

    fig, ax = plt.subplots(figsize=(8, 3.5))
    for k, lab in (("hueco_V", "victorias"), ("hueco_D", "derrotas")):
        if dd.get(k):
            ax.plot(range(len(dd[k])), dd[k], label=lab)
    ax.axhline(0, color="k", lw=.7); ax.set_xlabel("día"); ax.set_ylabel("su dinero − dinero del rival")
    ax.legend(); ax.grid(alpha=.3); ax.set_title("Hueco frente al rival, día a día")
    fig.tight_layout(); r = out_dir / f"{prefijo}_hueco.png"; fig.savefig(r, dpi=110); plt.close(fig)
    rutas["hueco"] = r

    fig, ax = plt.subplots(figsize=(8, 3.5))
    ax.plot(range(len(res["rigidez_dia"])), res["rigidez_dia"], marker="o")
    ax.set_ylim(0, 1.02); ax.set_xlabel("día"); ax.set_ylabel("fracción de partidas con la acción modal")
    ax.grid(alpha=.3); ax.set_title("Rigidez del guion: ¿hace lo mismo en todas las partidas?")
    fig.tight_layout(); r = out_dir / f"{prefijo}_rigidez.png"; fig.savefig(r, dpi=110); plt.close(fig)
    rutas["rigidez"] = r

    vd = res["ventas_dia"]
    if vd:
        prods = [k for k in PRODUCTOS if any(c.get(k) for c in vd.values())]
        fig, ax = plt.subplots(figsize=(9, 4))
        dias = sorted(vd)
        base = [0.0] * len(dias)
        for k in prods:
            ys = [vd[d].get(k, 0.0) for d in dias]
            ax.bar(dias, ys, bottom=base, label=k)
            base = [b + y for b, y in zip(base, ys)]
        ax.set_xlabel("día"); ax.set_ylabel("uds vendidas / partida"); ax.legend(ncol=3, fontsize=8)
        ax.set_title("Ventas ejecutadas por día y producto (media por partida)")
        fig.tight_layout(); r = out_dir / f"{prefijo}_ventas.png"; fig.savefig(r, dpi=110); plt.close(fig)
        rutas["ventas"] = r

    if rating:
        fig, ax = plt.subplots(figsize=(8, 3.5))
        ax.plot(range(len(rating)), [s for _, s in rating], marker=".")
        ax.set_xlabel("partida (orden cronológico)"); ax.set_ylabel("rating tras la partida"); ax.grid(alpha=.3)
        ax.set_title("Rating de la submission, partida a partida")
        fig.tight_layout(); r = out_dir / f"{prefijo}_rating.png"; fig.savefig(r, dpi=110); plt.close(fig)
        rutas["rating"] = r
    return rutas


EQUIPO = "青春猪头少年不会梦到kaggle金牌"
SUBMISSION = 56087555
EPISODIOS = [106623067, 106623966, 106624887, 106625788, 106626714, 106627641, 106628547, 106629461, 106630371, 106631286, 106632211, 106633142, 106634070, 106634996, 106635927, 106636869, 106637814, 106638780, 106639757, 106640711, 106641660, 106642605, 106643566, 106644498, 106645431, 106645717, 106646382, 106647321, 106648265, 106648844, 106649220, 106650159, 106650752, 106651126, 106652060, 106652170, 106653016, 106653967, 106654920, 106655854, 106656783, 106657837, 106658628, 106659613, 106660549, 106660991, 106661504, 106661603, 106662451, 106663381, 106663446, 106664315, 106665261, 106666196, 106667139, 106668068, 106668501, 106668760, 106668999, 106669956, 106670894, 106671842, 106672786, 106673722, 106674663, 106675612, 106676564, 106677469, 106678412, 106679280, 106680293, 106681231, 106682027, 106682180, 106683104]
CACHE = 'replays'   # carpeta local; en Kaggle se crea en /kaggle/working


partidas = []
for eid in EPISODIOS:
    raw = cargar_replay(eid, cache_dir=CACHE, pausa=1.0)
    yo = asiento_por_nombre(raw, EQUIPO)
    if yo < 0:
        continue
    partidas.append(analiza_partida(raw, yo, eid=eid))
res = resumen(partidas)
print(tabla_marcador(res))

# serie de rating: (fecha, updatedScore) tomada del listado público de episodios
import matplotlib.pyplot as plt
RATING = [["2026-09-08T00:52:29.067237Z", 600], ["2026-09-08T00:58:12.608413800Z", 677.0291466300902], ["2026-09-08T01:02:11.482077Z", 795.8604033779163], ["2026-09-08T01:06:12.786452900Z", 882.2617875330673], ["2026-09-08T01:10:11.381670800Z", 988.4430610243575], ["2026-09-08T01:14:15.581488200Z", 1069.808668051274], ["2026-09-08T01:18:12.975842800Z", 1136.7981167300643], ["2026-09-08T01:22:11.282832500Z", 1262.3380631336538], ["2026-09-08T01:26:14.524792200Z", 1374.4023765412153], ["2026-09-08T01:30:11.198337300Z", 1525.9567095741338], ["2026-09-08T01:34:13.325345Z", 1623.7365332264524], ["2026-09-08T01:38:12.984118300Z", 1754.6801599485734], ["2026-09-08T01:42:11.495370200Z", 1842.5905383159475], ["2026-09-08T01:46:11.841711700Z", 1928.8039430489198], ["2026-09-08T01:50:11.444129600Z", 2006.6115906836637], ["2026-09-08T01:54:11.515450Z", 2094.9040825485913], ["2026-09-08T01:58:12.776456400Z", 2151.4077508362775], ["2026-09-08T02:02:11.506810400Z", 2213.123030849629], ["2026-09-08T02:06:13.452709500Z", 2279.4461728845176], ["2026-09-08T02:10:11.288936600Z", 2347.0503049997596], ["2026-09-08T02:14:13.979532600Z", 2390.524513547719], ["2026-09-08T02:18:12.979745100Z", 2439.828713315911], ["2026-09-08T02:22:17.973114300Z", 2470.513396531736], ["2026-09-08T02:26:18.595079900Z", 2495.1504882954646], ["2026-09-08T02:30:17.634387800Z", 2531.1726530430074], ["2026-09-08T02:34:18.187069700Z", 2579.714355773901], ["2026-09-08T02:34:18.683677700Z", 2555.9994787495425], ["2026-09-08T02:38:17.603992400Z", 2600.7564375910342], ["2026-09-08T02:42:17.938010400Z", 2621.8071855753196], ["2026-09-08T02:46:18.428405100Z", 2620.210947264268], ["2026-09-08T02:46:19.392032Z", 2638.907402865795], ["2026-09-08T02:50:18.016167Z", 2639.574387215005], ["2026-09-08T02:54:17.916955900Z", 2667.7006744629234], ["2026-09-08T02:54:18.881090800Z", 2656.728240811341], ["2026-09-08T02:58:26.773034700Z", 2681.392598137712], ["2026-09-08T03:02:45.295405Z", 2669.757658499612], ["2026-09-08T03:02:45.479739400Z", 2656.7944556315756], ["2026-09-08T03:06:44.425499200Z", 2645.269456099985], ["2026-09-08T03:10:17.709960Z", 2635.917801865969], ["2026-09-08T03:14:17.315572200Z", 2645.692652682569], ["2026-09-08T03:18:17.813065100Z", 2656.5300604298304], ["2026-09-08T03:22:17.717052200Z", 2647.743446044651], ["2026-09-08T03:26:19.691383300Z", 2658.2720349519805], ["2026-09-08T03:30:17.479498100Z", 2651.5274031094873], ["2026-09-08T03:34:17.319903900Z", 2640.154594461152], ["2026-09-08T03:38:17.813615500Z", 2659.642873274044], ["2026-09-08T03:38:18.417305200Z", 2650.551560302675], ["2026-09-08T03:42:17.400175600Z", 2668.6273312885523], ["2026-09-08T03:42:17.537749400Z", 2675.219632050417], ["2026-09-08T03:46:20.484879Z", 2665.6819833609893], ["2026-09-08T03:50:17.907667Z", 2649.196758993903], ["2026-09-08T03:50:17.994651700Z", 2657.738511562439], ["2026-09-08T03:54:18.017283600Z", 2657.819218349781], ["2026-09-08T03:58:17.702440500Z", 2649.1703350932394], ["2026-09-08T04:02:17.389010700Z", 2642.2526739664686], ["2026-09-08T04:06:19.077992900Z", 2636.709672756581], ["2026-09-08T04:10:18.022048400Z", 2644.0579676037128], ["2026-09-08T04:10:18.798554800Z", 2649.3419257682335], ["2026-09-08T04:14:17.627190Z", 2643.738977247422], ["2026-09-08T04:14:18.013027700Z", 2648.7347604797847], ["2026-09-08T04:18:18.021030900Z", 2642.9242257367705], ["2026-09-08T04:22:18.437414800Z", 2634.629217058756], ["2026-09-08T04:26:21.835563600Z", 2626.9010125352156], ["2026-09-08T04:30:17.803000400Z", 2621.409425310303], ["2026-09-08T04:34:18.041353700Z", 2626.03893722177], ["2026-09-08T04:38:17.492773900Z", 2619.9431850890987], ["2026-09-08T04:42:18.532793200Z", 2623.7286752958908], ["2026-09-08T04:46:26.507321700Z", 2618.0801206777273], ["2026-09-08T04:50:18.128467200Z", 2621.985302813201], ["2026-09-08T04:54:17.445880400Z", 2625.5654899189294], ["2026-09-08T04:58:17.908671200Z", 2630.182624199347], ["2026-09-08T05:02:17.705979900Z", 2635.4883443416875], ["2026-09-08T05:06:21.431353400Z", 2640.568772538668], ["2026-09-08T05:10:17.890769400Z", 2631.9300763597184], ["2026-09-08T05:10:18.142595800Z", 2635.9218375668897], ["2026-09-08T05:14:18.207857800Z", 2627.715444801546]]
plt.figure(figsize=(8,3.5)); plt.plot([s for _, s in RATING], marker='.')
plt.xlabel('partida'); plt.ylabel('rating tras la partida'); plt.grid(alpha=.3); plt.show()

print(tabla_apertura(res))
print('contrataciones por paso:', res['contrataciones_pasos'])
print('tierra por día:', res['tierras'])
print('animales comprados (día, tipo):', res['animales_compra'])
print('primera venta, día:', res['primera_venta_dia'], '| producto:', res['primera_venta_item'])

rutas = figuras(res, 'figuras', 'xray')
from IPython.display import Image, display
display(Image(filename=str(rutas['rigidez'])))
print('divergencia entre pares:', res['divergencia_pares'])

print(tabla_grupos(res['por_tienda72'], 'tienda en t72'))
print()
print(tabla_granja(res))

display(Image(filename=str(rutas['dinero'])))
display(Image(filename=str(rutas['hueco'])))
print('día del peor hueco en derrotas:', res['peor_dia'])
print('día del mejor despegue en victorias:', res['mejor_dia'])
print('gasto medio en compra de producto:', round(res['gasto_pienso_medio'] or 0))

print(tabla_ventas(res))
display(Image(filename=str(rutas['ventas'])))
print('final de partida (pasos >= 700):', res['final'])
print('verbos por partida:', {k: round(v,1) for k, v in list(res['verbos_media'].items())[:10]})

print(tabla_grupos(res['por_clase'], 'clase de rival')); print()
print(tabla_grupos(dict(list(res['por_rival'].items())[:25]), 'rival (25 más frecuentes)')); print()
print(tabla_extremos(res['mayores_derrotas'], 'mayores derrotas')); print()
print(tabla_extremos(res['mayores_victorias'], 'mayores victorias'))