# Farming Score

The supplied four-route programme remains the policy backbone. Three public checkpoints select only prefix-compatible continuations, and the existing weed, shed-capacity, sell-clamping, and dead-stock guards remain unchanged. The revision adds one exact settlement rule at the final executable turn.


## Conserved routing and bounded repair

Let $R_j(t)$ denote route $j$ and let $c_t(x_t)$ select a continuation only when its entire prefix agrees with the active route. The programme can therefore change its future without inventing an incompatible past:

$$
R_{c_t(x_t)}(u)=R_j(u)
\qquad\text{for every }u<t.
$$

The checkpoints at turns 226, 360, and 433 use only public Yarn Store, carrot-price, and milk-inventory signals. Local repairs then reuse certain no-op turns, project same-turn shed deposits, protect one capacity slot at day close, and clamp infeasible sells. This combination is strong because adaptation is sparse while execution feasibility is checked continuously.


## The remaining terminal gap

The inherited dead-stock rule sells surplus only when the visible price exceeds $1$. That filter is sensible before the season ends because retaining an item can preserve future optionality. At the final executable turn $T=718$, however, continuation value is zero.

For projected post-unit-action shed quantity $q_i$ and market inventory $I_i$, terminal revenue is

$$
V_i(q_i;I_i)=\sum_{k=0}^{q_i-1}p_i(I_i+k).
$$

The engine enforces $p_i(I)\ge1$, hence

$$
q_i>0 \quad\Longrightarrow\quad V_i(q_i;I_i)>0.
$$

Selling therefore strictly dominates retaining any final product, including at the price floor.


## Terminal settlement rule

The revised controller uses the existing post-unit-action shed projection. At $T$, it expands each inherited product sell to the exact projected quantity, collapses duplicate product sells, and adds every missing positive-quantity product. The supplied terminal routes contain no non-sell market orders; with nine products and ten available slots, the full settlement fits.

Writing $S$ for this settlement operator, the intervention is

$$
\pi'_t=
\begin{cases}
\pi_t, & t<T,\\
(\text{same unit actions},\ S(M_T)), & t=T.
\end{cases}
$$

Thus route selection, the 99-item reserve, production, movement, purchases, and all turns through 717 are untouched. Running the code cell writes the standalone `main.py` and the upload-ready one-file `submission.tar.gz`.
