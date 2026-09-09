# Explicit hiring calendars

The original warm caller keeps inherited hire times. The first estimator assumed the earliest 39 free non-purchase order slots. These are different planning contracts even when every field action and output is identical.

The new development interface takes the public work obligations, 23 or 24 active phases, and a caller-supplied ordered list of up to 39 optional hire slots. A request for k workers uses the first k-1 slots. Slots must be unique, ordered, before the last active phase, and separate from fixed non-hire purchases. Source worker counts and hire events are ignored. A missing optional slot has zero action capacity and cannot be requested.

There are 289 fixed-size C++ features: the existing 233 physical features evaluated at the stated horizon, active hours, optional-slot count, 39 birth times, a combined necessary workforce bound, six carried-input descriptors, and eight seed/land release descriptors. Birth slots are part of the supplied plan, not reference answers. The pure path constructs no schedule and calls no solver. Future query models must use actual selected-worker capacity instead of the earlier constant 24+23*(k-1).

Identity has two levels. The obligation key excludes source workforce and hiring. The contract key hashes that obligation key, active horizon, and every optional slot. Reference evidence must never merge different calendars into one target. Family and parent grouping still prevents training on another calendar of a held-out physical day.

The first development panel contains 32 parents selected by physical hashes: eight source days, eight land/addition variants, eight small owned-tile additions, and eight terminal days. Each gets earliest, delay-three, delay-eight, late-hour-17, and two-hires-per-phase menus. The 160 contracts require 3,903 ordinary and 1,476 terminal queries. Query counts start at raw task-capacity lower bounds and end at the menu's full workforce, independently of source answers. Failures remain UNKNOWN at the fixed budget.

`data/calendar_development_v1/BASELINE_FREEZE.json` saves 560 forecasts before references start. One control ignores the calendar; another transfers the old cost-equivalent workforce through actual action capacity. This transfer is a heuristic. The frozen forest is used only for 24-phase inputs; terminal controls use necessary bounds and the route-capacity heuristic. The frozen unseen-family wave remains unchanged.

`check_planning` checks 119,104 explicit action capacities and invalid-menu controls. All 160 input extractions pass source-workforce and tile-order invariance. The feature timing includes physical extraction and the three bound calculations, excluding JSON loading, menu validation and the subsequent invariance checks. No next-version model or complete speed result is claimed yet.

The warm caller needs an additional context: inherited hires are mandatory, while extra hires may occur earlier. `planning_context.hpp` represents fixed slots first and optional slots second as a selection order, not worker IDs. Every selected schedule still executes hires chronologically. Its 290th feature is the explicitly committed workforce. A fixed hire in the last active phase is legal and costs money, but contributes zero action capacity. Optional zero-capacity hires remain excluded. The extended capacity/validation controls pass 119,111 checks.

The fixed-context extension was added after the first calendar forecasts and references started. `data/calendar_development_v1/reproducibility_addendum` records that timing, saves the resulting feature-source closure, and exactly reproduces all 289 original feature values on all 160 inputs. Original CSVs and forecasts remain untouched.
