"""Keep proven impossibility separate from the supported workforce cap."""


def screen_reason(row):
    if row["deadline_missing_quantity"] > 0:
        return "output_deadline_unreachable"
    if row.get("input_missing_quantity", 0) > 0:
        return "input_supply_unreachable"
    if row["lower_bound"] > 40:
        return "necessary_workforce_exceeds_40"
    return None
