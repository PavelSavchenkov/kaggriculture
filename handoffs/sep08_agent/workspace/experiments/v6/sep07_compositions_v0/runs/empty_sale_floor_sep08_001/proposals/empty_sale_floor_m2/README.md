# empty_sale_floor_m2

Late empty non-input sale removal with price-floor guard, mode2. Mode0 reproduces empty_sale_slots_m2. Mode1 protects later sales whose own requested volume reaches price1; mode2 adds matching rival sale volume; mode3 adds100 units for the rival shed capacity. Counts are observations and own requested trades only. Guards are heuristics: actual fills, rival purchases and rival supply are not known. All inherited behavior and source lineage retained. Experimental, not promoted. See ../../LINEAGE.json.
