# Farming Score: Terminal Settlement

The policy keeps the successful two-route production programme, its turn-360 shop-state decision, and its 72-turn affordability guard. The only behavioral revision is a complete product liquidation layer on the final executable turn.


## Why the baseline is strong

The baseline concentrates adaptation at one high-information checkpoint instead of repeatedly rewriting the plan. Let $R_0(t)$ and $R_1(t)$ be the two complete action tapes and let $g(x_{360})$ be the public-state route selector. Then

$$
\pi_t = R_{g(x_{360})}(t).
$$

Only the short interval $360 \le t \le 431$ differs between routes. Afterwards both paths rejoin the same production programme. This gives the agent three practical advantages: a proven long-horizon production backbone, limited exposure to noisy opponent inference, and predictable resource requirements that the block affordability guard can protect.


## Conservative terminal optimization

Let $T=718$ be the last executable turn, $q_i$ the sellable amount of product $i$ after unit actions resolve, and $p_i(I) \ge 1$ its market price. Unsold inventory has no terminal value, so

$$
V_i(q_i;I_i)=\sum_{k=0}^{q_i-1}p_i(I_i+k) > 0
\qquad\text{for every }q_i>0.
$$

Therefore selling all executable final inventory weakly dominates retaining any of it. The engine caps a sell by available inventory, so a large request $Q=10^6$ executes as

$$
q_i^{\mathrm{exec}}=\min\!\left(Q,q_i\right).
$$

At $T$, every existing product sell is expanded to $Q$, then missing product types receive one sell order each. There are nine sellable products and ten market slots, so the entire product set fits. Expanding existing orders is important: it also captures additional milk or wheat deposited by a same-turn `DROP`, which a missing-products-only sweep can overlook.


## Intervention scope

Writing $M_T$ for the original final market list and $S(M_T)$ for the settlement sweep, the revised policy is

$$
\pi'_t =
\begin{cases}
\pi_t, & t<T,\\
(\text{same unit actions},\ S(M_T)), & t=T.
\end{cases}
$$

No route threshold, movement, planting, care, harvest, purchase, or nonterminal sale is changed. Running the code cell writes a standalone `main.py` and an upload-ready `submission.tar.gz` containing only that file.
