# DayIntent

## Fields

| Scope | Field | Type and size | Meaning |
|---|---|---|---|
| **Whole farm fields** |  |  |  |
| Whole farm | `new_wheat_count` | Integer, 0–100 | Wheat planted today and alive after tonight's update |
| Whole farm | `new_carrot_count` | Integer, 0–100 | Carrots planted today and alive after tonight's update |
| Whole farm | `new_tomato_count` | Integer, 0–100 | Tomatoes planted today and alive after tonight's update |
| Whole farm | `new_strawberry_count` | Integer, 0–100 | Strawberries planted today and alive after tonight's update |
| Whole farm | `new_melon_count` | Integer, 0–100 | Melons planted today and alive after tonight's update |
| Whole farm | `new_goose_count` | Integer, 0–100 | Geese placed today and alive after tonight's update |
| Whole farm | `new_cow_count` | Integer, 0–100 | Cows placed today and alive after tonight's update |
| Whole farm | `new_sheep_count` | Integer, 0–100 | Sheep placed today and alive after tonight's update |
| Whole farm | `unplaced_goose_after_tonight_count` | Integer, 0–100 | Exact number of unplaced geese in the shed after tonight's update |
| Whole farm | `unplaced_cow_after_tonight_count` | Integer, 0–100 | Exact number of unplaced cows in the shed after tonight's update |
| Whole farm | `unplaced_sheep_after_tonight_count` | Integer, 0–100 | Exact number of unplaced sheep in the shed after tonight's update |
| Whole farm | `buy_next_land_today` | Boolean | Whether to buy the next land quadrant today |
| **One-shot crop-group fields** |  |  |  |
| Each existing or new one-shot crop group | `daily_action_counts` | Nine integer counts | Exact number of crops assigned to each daily option: one of the eight combinations of today's three actions, or `clear` |
| **Ongoing crop-group fields** |  |  |  |
| Each existing ongoing crop group | `retain_after_tonight_count` | Integer, 0–group size | Exact number of existing crops that must still be alive after tonight's update |
| Each existing ongoing crop group | `clear_today_count` | Integer, 0–group size | Exact number of non-retained crop tiles that must become free for reuse during this day |
| Each existing ongoing crop group | `fertilize_today_count` | Integer, 0–retained count | Exact number of retained crops to fertilize today |
| Each existing ongoing crop group | `harvest_today_count` | Integer, 0–group size | Exact number of crops whose currently held product must be harvested today |
| **Animal-group fields** |  |  |  |
| Each existing or new animal group | `feed_today_count` | Integer, 0–group size | Number of animals to feed today |
| Each existing or new animal group | `care_today_count` | Integer, 0–group size | Number of animals to both feed and care for today |
| Each existing or new animal group | `collect_today_count` | Integer, 0–group size | Number of animals whose held egg, milk, or wool must be collected today |

`DayIntent` is the network's request for one day. It describes useful outcomes
and cross-day commitments. It is not an action list.

Every count is exact unless its description explicitly calls it a minimum.

A valid `DayIntent` satisfies every constraint in this document, including
constraints that link several fields, such as group partitions, count bounds,
and enough free tiles and shed space for the requested crops and animals.
Training losses and decoding must enforce these constraints.

The compiler that executes this interface is specified in
[`day_compiler.md`](day_compiler.md).

### Fixed values

In some states a field or option has only one sensible value: its action is
impossible, contradicts the field's definition, has no use because the game
ends, or is proven worthless by the game rules. That value is fixed. It is
decided from the dawn state alone and is zero unless a rule says otherwise.

Fixed values are handled the same way during dataset generation, training,
inference, and compilation:

- decoding never chooses another value;
- the network receives no training loss for them;
- metrics leave them out; and
- the compiler receives only the fixed value.

A fixed value is never a fallback for an unrepresentable schedule. If a
converted label disagrees with a fixed value, the interface-coverage gate fails.

These are all fixed-value rules.

On day 29, the game ends before tonight's update:

- all eight `new_*_count` fields are zero;
- each `unplaced_*_after_tonight_count` equals the unplaced animals of that
  species held at dawn, so no animal is bought;
- ongoing-crop `retain_after_tonight_count` and `clear_today_count` are zero,
  and the retain, clear, and abandon partition is not used; and
- one-shot `clear` and every `feed_today_count` are zero.

New groups:

- When a new group's `new_*_count` is zero, every other field and option of
  that group is zero.
- In a new one-shot group, `clear` and every option without water are zero.
  Every member must be alive after tonight's update.

Impossible actions:

- In a one-shot group too young to harvest, every option with harvest is zero.
- With no held product, ongoing-crop `harvest_today_count` and animal
  `collect_today_count` are zero.

Worthless actions:

- A one-shot option is zero when one of its actions cannot change present or
  future yield or survival, given the option's other actions. Examples are
  fertilize and harvest without water, water with harvest when water adds no
  yield today, and water or fertilize without harvest on a crop that decays
  today or on day 29.
- Ongoing-crop `fertilize_today_count` is zero when fertilizing today does not
  extend fertilizer activity or no production can use it.
- `care_today_count` is zero when no later production can use today's care
  before the game ends.

Counts that partition a group sum to the group size.

## DayIntent groups

Existing crop groups are ordered lexicographically by crop type, age, current
harvest yield or held product, consecutive dry days, and remaining fertilizer
days. The new crop groups follow them in wheat, carrot, tomato, strawberry, and
melon order.

Remaining fertilizer days is the number of days, starting today, on which
fertilizer is active on the crop: 0 if it is not active today, 1 if the crop was
fertilized two days ago, and 2 if it was fertilized yesterday. A crop whose
fertilizer has expired and a crop never fertilized both have 0.

Existing animal groups are ordered lexicographically by species, age, held
product, consecutive unfed days, and banked care bonus. The new animal groups
follow them in goose, cow, and sheep order.

### One-shot crop groups

Wheat, carrots, and melons form one-shot crop groups. An existing group contains
crops that are fully identical except for their tile locations. In particular,
all of these facts are equal:

- crop type;
- age in days;
- current harvest yield;
- consecutive dry days;
- remaining fertilizer days.

Current harvest yield is the quantity the crop would return if harvested now,
without more work.

For each one-shot crop group, the network emits:

- `daily_action_counts`: one count for each of nine options.

Eight options are the combinations of three Boolean actions, `water`,
`fertilize`, and `harvest`, from no action to all three. The ninth, `clear`,
digs the crop without harvesting it, so its tile becomes free for reuse today.
`clear` is not combined with other actions: a crop dug without harvest counts
under `clear` whatever else was done to it. The counts record which actions
apply to the same crop while leaving exact tiles and action times to the
compiler.

A one-shot crop leaves its tile only by harvest, by `clear`, or by becoming a
weed. The compiler never digs any other one-shot crop.

The new group for each one-shot crop type starts at age 0.

### Ongoing crop groups

Tomatoes and strawberries form ongoing crop groups. One existing group contains
crops that are fully identical except for their tile locations. In particular,
all of these facts are equal:

- crop type;
- age in days;
- held product already produced and waiting for harvest;
- consecutive dry days;
- remaining fertilizer days.

For each existing ongoing crop group, the network emits:

- `retain_after_tonight_count`;
- `clear_today_count`;
- `fertilize_today_count`;
- `harvest_today_count`.

`retain_after_tonight_count` is the exact number of crops from the group that
must still be alive after tonight's update. `clear_today_count` is the exact
number of other crops whose tiles must become free for reuse during this day.
The remaining crops are abandoned:

`abandon_count = group_size - retain_after_tonight_count - clear_today_count`.

`abandon_count` is the exact number of current crop tiles that are weeds after
tonight's update and were not cleared today. This includes crops that decay
into weeds during the day. If a group cannot become weeds by then,
`abandon_count` must be zero and `retain + clear` must equal the group size.

The tile counts as cleared even if the compiler later replants it or builds on
it.

`fertilize_today_count` and `harvest_today_count` are also exact. A crop may be
harvested even when it will not be retained. A fertilized crop must be one the
compiler keeps.

The new tomato and strawberry groups start at age 0 and have sizes
`new_tomato_count` and `new_strawberry_count`. They have no additional output
fields. The compiler waters every member so it survives tonight. Same-day
fertilizer is omitted because it expires before either crop's first production.

### Animal groups

One existing animal group contains animals that are fully identical except for
their tile locations. In particular, all of these facts are equal:

- species;
- age;
- held product;
- consecutive unfed days;
- banked care bonus.

Each group contains the animals' tile list, but location is not part of the
group key.

For each existing or new animal group, the network emits three counts:

- `feed_today_count`;
- `care_today_count`; and
- `collect_today_count` for eggs, milk, or wool.

Each count is at most the group size, and care cannot exceed feed.

Retention is not an output because feeding already fixes it. Animals with zero
consecutive unfed days cannot escape tonight. In a group with one unfed day,
fed animals survive tonight and the others escape.

The new goose, cow, and sheep groups start at age 0. Their sizes are
`new_goose_count`, `new_cow_count`, and `new_sheep_count`. Every member is alive
after tonight's update.

Unplaced animals do not form animal groups. They have no age, tile, feeding,
care, production, or collection decisions. For each species,
`unplaced_*_after_tonight_count` is the exact total that must remain unplaced in
the shed after tonight's update. All carried inventory is returned to the shed
during that update.

The compiler does not deliberately discard animals. Therefore, for each
species:

`new_*_count + unplaced_*_after_tonight_count >= unplaced animals held at dawn`.

The required number of successful purchases is:

`new_*_count + unplaced_*_after_tonight_count - unplaced animals held at dawn`.

The three after-tonight reserve counts, together with every other item retained
in the shed, must fit within shed capacity. Exact purchase orders and purchase
hours remain compiler decisions.

## Conversion to DayIntent

The converter has one job:

`(dawn state, history, original schedule for the coming day) -> DayIntent`

The history contains everything observed before that dawn. The original
schedule contains the worker actions and market orders for the coming day. The
converter replays the schedule with the exact game engine, so it also knows
which actions succeed and what state they produce.

The converter builds the same crop and animal groups that will be built at
runtime. It then records the original schedule exactly for every decision owned
by `DayIntent`.

For each animal reserve field, the label is the corresponding animal count in
the shed after tonight's automatic inventory return. It is not the number of
animals purchased during the day.

Each count is recorded by its definition, so work that no count includes, such
as care on an unfed animal, is not recorded. Before recording, the converter
drops every action that a [fixed-value](#fixed-values) rule excludes. For
example, fertilizer before a same-day harvest without water is dropped, leaving
harvest-only.

Labels use only the dawn state, prior history, and the coming day's schedule
and resulting state. They never use later days.

The converter ignores an entire day only in these two cases:

- more than one land quadrant is successfully bought during the day; or
- an unplaced animal is discarded during the day or tonight's inventory return.

Dataset generation reports the number of ignored days for each reason. It must
not clamp either case into a representable label.

### No silent fallback

Every valid day schedule that is not explicitly ignored above must be
representable by `DayIntent` in this way. The converter must not guess, clamp a
count, choose the closest option, replace a missing value with zero, or use a
default intent.

If any other valid schedule cannot be represented exactly, dataset generation
must report the exact schedule pattern and fail its interface-coverage gate.
It must then stop and explicitly ask the user what to do. The user may choose
to extend the interface or add a fixed-value rule after reviewing proof that
the excluded work has zero value. Training must not proceed with a fabricated
label or without the user's decision.
