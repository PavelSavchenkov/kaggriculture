"""Offline calibration diagnostics; no candidate selection or policy search."""
import json
from pathlib import Path

import numpy as np
import pandas as pd

EXP=Path(__file__).resolve().parents[1]


def main():
    estimates=pd.read_csv(EXP/"results/estimated_library.csv")
    rows=[]
    for mode in (0,1,2):
        for path in sorted((EXP/f"results/library_{mode}").glob("program_*.json")):
            data=json.loads(path.read_text())
            row={"program":int(data["agent_a"].split("_")[1]),"compiler_service":mode,
                **{key:data[key] for key in ("mean_cash","mean_margin","win_utility","cash_cvar10","margin_cvar10")}}
            for item in range(9):row[f"actual_produced_{item}"]=np.mean([g["produced"][item] for g in data["games"]])
            row["faults"]=sum(g["unit_faults"] for g in data["games"])
            row["phenotype"]="_".join(g["action_hash"] for g in data["games"])
            rows.append(row)
    exact=pd.DataFrame(rows)
    exact.to_csv(EXP/"results/compiler_library.csv",index=False)
    reports=[]
    for mode in (0,1,2):
        # Productive compiler still fertilizes ongoing crops; mode 0 is an
        # explicitly mismatched first proxy. Source mode uses identical masks.
        a=exact[exact.compiler_service==mode]
        for estimate_mode in (0,1,2):
            for support in (0,1):
                e=estimates[(estimates.service==estimate_mode)&(estimates.recorded_support==support)&(estimates.recorded_layout==1)]
                joined=a.merge(e,on="program").drop_duplicates("phenotype")
                predicted=joined.conditional_cash.to_numpy();actual=joined.mean_cash.to_numpy()
                concordant=valid=0
                for i in range(len(joined)):
                    for j in range(i):
                        x=predicted[i]-predicted[j];y=actual[i]-actual[j]
                        if x and y:valid+=1;concordant+=(x*y>0)
                reports.append({"compiler_service":mode,"estimate_service":estimate_mode,"recorded_support":support,
                    "distinct_phenotypes":len(joined),"spearman_cash":joined.conditional_cash.corr(joined.mean_cash,method="spearman"),
                    "spearman_margin":joined.conditional_cash.corr(joined.mean_margin,method="spearman"),
                    "spearman_two_sided_margin":joined.conditional_margin.corr(joined.mean_margin,method="spearman"),
                    "pairwise_cash_concordance":concordant/valid,"mean_cash_error":float(np.mean(predicted-actual)),
                    "cash_mae":float(np.mean(abs(predicted-actual))),
                    "scope":"72 replay programs, 8 discovery games each; deduplicated by joint scenario action hashes. Fixed teacher-game rival flows; conditional forecasts with funding gaps."})
    (EXP/"results/estimator_calibration.json").write_text(json.dumps(reports,indent=2)+"\n")
    print(pd.DataFrame(reports).drop(columns="scope").round(3).to_string(index=False))
    print("Exact mean cash and margin",exact.groupby("compiler_service")[["mean_cash","mean_margin","win_utility","faults"]].mean().to_string())


if __name__=="__main__":main()
