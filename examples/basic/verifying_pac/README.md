# PAC verification

These scenarios show how to verify different kinds of Probabilistic Approximate Constraints (PACs).
For each kind of PAC we start with basic usage and then show more advanced concepts such as defining
custom metrics.
It is highly recommended to read these examples in order.

## Domain PAC
+ [verifying_domain_pac1.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/verifying_pac/verifying_domain_pac1.py) — a scenario showing how to verify a Domain PAC over a single column, and how to gain additional knowledge by inspecting outliers.
+ [verifying_domain_pac2.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/verifying_pac/verifying_domain_pac2.py) — a scenario showing how to verify a Domain PAC over multiple columns using a built-in Parallelepiped domain.
+ [verifying_domain_pac3.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/verifying_pac/verifying_domain_pac3.py) — a scenario showing how to verify a Domain PAC over multiple columns using a built-in Ball domain.
+ [verifying_domain_pac4.py](https://github.com/Desbordante/desbordante-core/tree/main/examples/basic/verifying_pac/verifying_domain_pac4.py) — a scenario showing how to verify a Domain PAC over a column with string values using Levenshtein distance.
