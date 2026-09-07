# Results

Final held-out stratified audit: seeds `449000001..449002048`, both seats, every
shop forced once per seed, random weeds `0.005`, PASS opponent, and later shop
unlocks disabled. This is 4,096 games per shop and 32,768 games total.

```text
overall mean 122794.218903  CVaR10 107444.379089  J 119724.250940
failures 0  discards 0

shop             dynamic mean   oracle - dynamic   dynamic - no-shop
Bakery           108338.817383          -21.000000          405.931152
Brunch           114815.967041         -181.000000         2438.080811
Farmers Market   114736.447021         -185.999512         2188.560791
Ice Cream        144180.006836         -344.099609        25037.120605
Pet Cafe         107447.103271          -20.313232          389.217041
Pizza            126382.190186         -902.269043        11408.303955
Smoothie         142725.633057         -350.053711        25069.749756
Yarn             123727.586426         1523.985840        12426.794434
```

The oracle column uses the strongest prior agent that knew its shop from turn
zero. Negative gaps mean this dynamic policy is better on the audit block. The
no-shop column compares against `fixed_weed_105492_robust`, the strongest
sealed no-shop/random-weed policy; positive values favor the dynamic policy.

Release and ASan/UBSan checks cover both seats and all shops. The Kaggle Python
compiler reproduces all 16 deterministic target games action-for-action after
normalizing one zero-quantity market no-op, and reproduces every terminal cash
exactly.
