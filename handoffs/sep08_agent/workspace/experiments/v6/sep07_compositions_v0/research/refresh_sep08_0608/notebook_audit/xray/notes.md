# Radiografía pública del agente de yamakawanin en Kaggriculture (submission 56087555)

Este cuaderno analiza, **solo con datos públicos**, el agente que el equipo **青春猪头少年不会梦到kaggle金牌** ([yamakawanin](https://www.kaggle.com/yamakawanin)) tiene en la clasificación de [Kaggriculture](https://www.kaggle.com/competitions/kaggriculture/leaderboard). Kaggle publica el replay completo de cada episodio (`https://www.kaggleusercontent.com/episodes/<id>.json`): las observaciones de los dos asientos y la acción que cada agente devolvió en cada uno de los 720 pasos. Con eso basta para saber qué hace un agente, cuándo lo hace, si lo hace siempre igual y contra quién gana o pierde. No se re-simula nada y no se usa información privada.

El cuaderno está pensado para poder re-ejecutarse tal cual (activando Internet en la sesión de Kaggle): la librería de análisis va incrustada en la primera celda de código y las cifras del texto salen de ella.

## Ficha

| | |
|---|---|
| usuario Kaggle | [yamakawanin](https://www.kaggle.com/yamakawanin) |
| equipo | 青春猪头少年不会梦到kaggle金牌 (id 16632224) |
| submission analizada | 56087555 (subida 2026-09-08 00:52 UTC) |
| rating tras su última partida pública (API `ListEpisodes`) | 2.627,7 (2026-09-08 05:14 UTC) |
| partidas públicas de esa submission | 76 (validación incluida: 1) · analizadas: 75 |
| puesto / rating en el censo del 2026-09-07T19:37:41 | - / - |
| submissions distintas del equipo vistas en el ladder | 13 |
| clasificación | [https://www.kaggle.com/competitions/kaggriculture/leaderboard](https://www.kaggle.com/competitions/kaggriculture/leaderboard) |

### Submissions del mismo equipo vistas en el ladder

| submission | partidas vistas | primera | última |
|---|---:|---|---|
| 55879996 | 1 | 2026-08-31T12:02 | 2026-08-31T12:02 |
| 55935089 | 1 | 2026-09-01T10:22 | 2026-09-01T10:22 |
| 55935225 | 1 | 2026-09-01T10:34 | 2026-09-01T10:34 |
| 55960693 | 1 | 2026-09-02T13:00 | 2026-09-02T13:00 |
| 55961805 | 1 | 2026-09-02T17:39 | 2026-09-02T17:39 |
| 55973040 | 1 | 2026-09-03T06:10 | 2026-09-03T06:10 |
| 55980347 | 6 | 2026-09-03T14:58 | 2026-09-04T08:45 |
| 56012679 | 2 | 2026-09-04T15:29 | 2026-09-05T10:10 |
| 55980366 | 1 | 2026-09-05T12:10 | 2026-09-05T12:10 |
| 56036218 | 2 | 2026-09-06T01:15 | 2026-09-06T03:11 |
| 56046688 | 15 | 2026-09-06T06:19 | 2026-09-07T02:27 |
| 56049529 | 18 | 2026-09-06T08:51 | 2026-09-07T06:43 |
| 56072271 | 3 | 2026-09-07T13:04 | 2026-09-07T21:34 |


## 0. Librería de análisis

Todo lo que sigue usa estas funciones (solo librería estándar; matplotlib para las figuras). Se lee un replay, se elige el asiento del equipo y se recorre paso a paso: órdenes de mercado (pedido y **ejecutado** = mín(pedido, lo que había en el cobertizo)), acciones físicas, dinero al cierre de cada día, cultivos, animales, tiendas.

## 1. Partidas analizadas

Los 75 episodios públicos de la submission 56087555 (sin contar la partida de validación, que es el agente contra sí mismo). Cada id enlaza al visor de Kaggle: `https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-<id>`.

Sobre 75 partidas públicas la submission va **51-24-0** (tasa 68.0 %), margen medio 12.353 y mediano 2.831, dinero final medio 100.749. Estados de su agente al terminar: DONE ×75.

### Rating partida a partida

El `updatedScore` que Kaggle asigna a su agente tras cada episodio, en orden cronológico (lo devuelve la API pública `ListEpisodes`).

## 2. Apertura: qué pide al mercado en los primeros pasos

Variantes de la lista `market` que devuelve en los pasos 0-5, con cuántas partidas usan cada una. Si una variante aparece en todas las partidas, esa decisión no depende del rival ni del mundo.

En el paso 0 hay 1 variante(s) de órdenes de mercado; la más frecuente, `[["BUY_PRODUCT","WHEAT",13]]`, aparece en 75 de 75 partidas. En el paso 1 hay 1 variante(s) (la principal en 75 de 75). Apertura fija en los dos primeros pasos: no reacciona al rival hasta más tarde.

Las manos se contratan cada día: contrata de media 280,0 veces por partida, sobre todo en los pasos t1 (5,0/partida), t24 (4,0/partida), t48 (4,0/partida), t72 (5,0/partida), t96 (4,0/partida), t120 (3,0/partida), t121 (2,0/partida), t144 (7,0/partida) …. Compra tierra en los días 6 (1,00/partida), 11 (1,00/partida). Animales comprados (día y tipo → unidades por partida): d0 COW 2,00, d0 SHEEP 2,00, d2 COW 1,00, d3 COW 1,00, d6 COW 1,44, d6 GOOSE 0,84, d7 COW 1,80, d7 SHEEP 0,20, d8 COW 0,52, d8 SHEEP 2,48, d9 SHEEP 0,40, d10 GOOSE 2,24, d10 SHEEP 0,56, d11 SHEEP 1,00.

Primera venta ejecutada: día 0 (75 partidas); producto: WHEAT ×75.

## 3. ¿Guion fijo o decisiones? Rigidez del comportamiento

Para cada paso se mira qué acción (granjero + manos + mercado, completa) es la más común entre sus partidas y qué fracción de partidas la repite exactamente. Promediado por día da la curva de abajo. Además, para cada par de partidas se busca el primer paso con acción distinta.

Rigidez del guion (fracción de partidas que hacen exactamente la acción modal en cada paso, promediada por día): día 0 100.0 %, día 1 100.0 %, día 3 100.0 %, día 7 91.7 %, día 15 97.4 %, día 28 61.7 %. Entre pares de partidas (2775 pares), la primera acción distinta aparece de mediana en el paso 149 (mín 149, máx 695); por día del primer desvío: d6: 1719, d9: 140, d10: 124, d15: 325, d16: 160, d17: 108, d18: 35, d19: 62, d20: 18, d21: 26, d22: 47, d24: 2, d25: 3, d27: 2, d28: 4. Una rigidez alta durante muchos días es la marca de un guion pre-grabado; el desvío temprano marca dónde entra la primera decisión que depende del mundo (precios, tienda, rival).

## 4. Ruta según la tienda del pueblo y granja día a día

La primera tienda se desbloquea en el paso 72 y condiciona qué producto tiene demanda. Abajo, su marcador según esa tienda y la evolución media de su granja (manos, cuadrantes, pastos, corrales, animales y casillas sembradas por cultivo).

Primera tienda del pueblo (paso 72) en sus partidas: FARMERS_MARKET ×14, PET_CAFE ×11, PIZZA_SHOP ×11, BAKERY ×9, SMOOTHIE_SHOP ×8, BRUNCH_SPOT ×8, ICE_CREAM_SHOP ×8, YARN_STORE ×6.

## 5. Economía: dinero día a día y dónde se abre el hueco

Dinero al cierre de cada día (media), separando las partidas que gana de las que pierde, y el hueco frente al rival (su dinero menos el del rival) en unas y otras.

Dinero medio al cierre del día 8: 680; día 16: 33.839; día 24: 75.441; final: 100.749. En las derrotas, el día en que más se le abre el hueco (día con peor variación del hueco): d10 ×2, d13 ×1, d15 ×4, d19 ×1, d20 ×1, d21 ×1, d24 ×1, d25 ×2, d26 ×2, d28 ×2, d29 ×7. En las victorias, el día en que más se despega: d9 ×1, d10 ×9, d11 ×1, d14 ×2, d15 ×1, d16 ×5, d18 ×4, d20 ×1, d21 ×1, d22 ×5, d24 ×1, d26 ×6, d28 ×8, d29 ×6. Gasto medio por partida en productos comprados en el mercado (pienso y similares): 12.965.

## 6. Mercado: qué vende, cuánto pide y cuánto se ejecuta

«Pedido» es la cantidad de la orden SELL; «ejecutado» es lo que de verdad había en el cobertizo en ese momento (el mercado no vende lo que no existe). La diferencia entre ambas describe el estilo de las órdenes.

Vende de media por partida: FERTILIZER 288,7 uds, WHEAT 273,5 uds, MILK 153,2 uds, STRAWBERRY 94,7 uds, EGG 62,2 uds, WOOL 58,8 uds. Valor medio de lo vendido por partida: 59.957. Órdenes de mercado por partida: 730,6. Pide mucho más de lo que tiene en WHEAT, CARROT, STRAWBERRY, MELON, MILK, WOOL (pedido > 3× ejecutado): es el patrón de «vuelca todo lo que haya» con cantidades grandes. Final de partida (pasos ≥ 700): 9,7 DROP y 9,5 órdenes SELL por partida (5,6 uds).

## 7. Contra quién gana y contra quién pierde

Cada rival se clasifica comparando su tablero con el de él a la hora 20 de los días 0-3 (posición del granjero y de las manos). Coincidir en las cuatro es jugar el mismo guion.

Clasifico a cada rival por si su tablero (posición del granjero y de las manos a la hora 20 de los días 0-3) coincide con el suyo: **gemelo** (las cuatro coinciden), **misma_apertura** (solo el día 0) o **distinto**. Resultado: gemelo: 45 partidas, 28-17-0, margen medio 339; distinto: 19 partidas, 14-5-0, margen medio 39.496; misma_apertura: 11 partidas, 9-2-0, margen medio 14.619. Equipos gemelos vistos: 37 (An - Claudex, Ankit Hemant Lade, ChengxuGu, David Estevez, David Goldrajch, Devansh3007, FarmGoose, GPT-5.6 Sol🏆, GURU Prasaatha S, GanadorPlusUltra, JOMIN URK21CS1077, Jeryos…); equipos con su misma apertura: 11. El paso en que sus acciones dejan de coincidir con las del rival, por día: d0: 44, d6: 25, d9: 6.

## 8. El fichero que publicó

Publicado en [https://www.kaggle.com/code/yamakawanin/kaggriculture-2312-9-q30-v2a](https://www.kaggle.com/code/yamakawanin/kaggriculture-2312-9-q30-v2a). Fichero: `yamakawanin_main.py`, 459.800 bytes, 1.069 líneas, sha256 `d03897161859873f2316e2419f3b9a05857b09905b6505c2652e7a6983a021aa`, md5 `31d87cc291f50db06c445db643855b5e`. Define 49 funciones, reasigna `agent` 10 veces (la última a `q30_v2a`) y contiene 3 bloque(s) codificado(s) en base64/base85 con 410.414 caracteres en total.

### Otras submissions del mismo equipo (API pública)

| submission | subida (UTC) | partidas públicas | V-D | rating tras la última |
|---|---|---:|---|---:|
| 56072271 | 2026-09-07 07:53 | 146 | 103-43 | 2.406,6 |

### ¿Es el fichero publicado el que juega en vivo?

Se re-juega el mismo mundo (misma semilla, `kaggle_environments` 1.32.7) con su fichero en su asiento y el rival reproduciendo exactamente las acciones grabadas en el replay; en la partida de validación los dos asientos llevan su fichero. Si el agente es determinista y es el mismo, las 719 acciones y el dinero final coinciden. `t0` es la lista `market` del paso 0.

| submission | episodio | asiento(s) con su fichero | pasos iguales | primer paso distinto | t0 en vivo | t0 re-jugado | reward real | reward re-jugado | seg. |
|---|---|---|---|---|---|---|---|---|---:|
| 56087555 | [106621962](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106621962) | [0, 1] | 20/719, 20/719 | 0, 0 | `[["BUY_PRODUCT", "WHEAT", 13]]; [["BUY_PRODUCT", "WHEAT", 13]]` | `[["BUY_PRODUCT", "WHEAT", 30]]; [["BUY_PRODUCT", "WHEAT", 30]]` | [66620.0, 67376.0] | [57697.0, 58418.0] | 8.3 |
| 56087555 | [106623067](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106623067) | [0] | 20/719 | 0 | `[["BUY_PRODUCT", "WHEAT", 13]]` | `[["BUY_PRODUCT", "WHEAT", 30]]` | [155669.0, 30926.0] | [153550.0, 32620.0] | 5.8 |
| 56087555 | [106623966](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106623966) | [1] | 18/719 | 0 | `[["BUY_PRODUCT", "WHEAT", 13]]` | `[["BUY_PRODUCT", "WHEAT", 30]]` | [67610.0, 135157.0] | [79199.0, 118438.0] | 7.3 |
| 56087555 | [106624887](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106624887) | [1] | 51/719 | 0 | `[["BUY_PRODUCT", "WHEAT", 13]]` | `[["BUY_PRODUCT", "WHEAT", 30]]` | [28355.0, 105962.0] | [57942.0, 130627.0] | 7.0 |
| 56072271 | [106387660](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106387660) | [1] | 705/719 | 178 | `[["BUY_PRODUCT", "WHEAT", 30]]` | `[["BUY_PRODUCT", "WHEAT", 30]]` | [26919.0, 121775.0] | [26919.0, 121258.0] | 5.0 |
| 56072271 | [106388549](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106388549) | [1] | 116/719 | 72 | `[["BUY_PRODUCT", "WHEAT", 30]]` | `[["BUY_PRODUCT", "WHEAT", 30]]` | [45163.0, 156478.0] | [69425.0, 150618.0] | 6.8 |
| 56072271 | [106389479](https://www.kaggle.com/competitions/kaggriculture/leaderboard?dialog=episodes-episode-106389479) | [1] | 718/719 | 385 | `[["BUY_PRODUCT", "WHEAT", 30]]` | `[["BUY_PRODUCT", "WHEAT", 30]]` | [36962.0, 107410.0] | [36976.0, 107396.0] | 5.3 |

Submission 56087555: reproduce paso a paso 0 de 4 partidas re-jugadas; primer paso distinto en 0, 0, 0, 0, 0 (desde el paso 0: la apertura en vivo no es la del fichero). Submission 56072271: reproduce paso a paso 0 de 3 partidas re-jugadas; primer paso distinto en 72, 178, 385 (coincide la apertura y se separa después: otra versión del mismo guion, o un componente que depende de algo que el re-juego no reproduce).

## Resumen

- **Marcador:** 51-24-0 en 75 partidas públicas (tasa 68.0 %).
- **Apertura:** 1 variante(s) en t0 y 1 en t1.
- **Rigidez:** primera acción distinta entre pares de partidas, mediana paso 149.
- **Economía:** dinero final medio 100.749; en derrotas el hueco se abre sobre todo el día 29.
- **Mercado:** vende sobre todo FERTILIZER, WHEAT, MILK.
- **Rivales:** gemelo 28-17; distinto 14-5; misma_apertura 9-2.

Todo lo anterior son cuentas sobre replays públicos; con pocas partidas por rival (n = 1-3) las filas por rival son anécdota, no regla. Las cifras cambian a medida que la submission juega más partidas: la fecha de generación está al principio.