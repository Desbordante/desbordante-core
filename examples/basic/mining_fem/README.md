## Frequent episode mining (FEM)

These scenarios showcase Desbordante's frequent episode mining (FEM) algorithms, which mine sequential data.

Read [afem.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/mining_fem/afem.py) first: it introduces the sequence dataset format, the composite/parallel/serial episode notation, and the occurrence & support definitions the other two examples build on.

+ [afem.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/mining_fem/afem.py) — discovers every frequent episode (AFEM) given a minimum support threshold.
+ [maxfem.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/mining_fem/maxfem.py) — discovers only the maximal frequent episodes (MaxFEM), a lossless, typically much smaller summary of AFEM's output.
+ [tke.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/mining_fem/tke.py) — discovers the top-k most frequent episodes (TKE), replacing the minimum support threshold with a target count k.
