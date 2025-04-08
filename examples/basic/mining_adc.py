import desbordante as db
import pandas as pd

RED = '\033[31m'
YELLOW = '\033[33m'
GREEN = '\033[32m'
CYAN = '\033[1m\033[36m'
ENDC = '\033[0m'

TABLE_1 = "examples/datasets/taxes_1.csv"
TABLE_2 = "examples/datasets/taxes_2.csv"

def print_table(filename: str, title: str = "") -> None:
    if title:
        print(f"{title}")
    data = pd.read_csv(filename, header=0)
    print(data, end="\n\n")

def main():
    print(f"""{YELLOW}Understanding Denial Constraints (DCs){ENDC}
In this walkthrough, we follow the definitions described in the paper
\"Fast approximate denial constraint discovery\" by Xiao, Tan,
Wang, and Ma (2022) [Proc. VLDB Endow. 16(2), 269–281].

A Denial Constraint is a statement that says: "For all pairs of different rows in a table,
it should never happen that some condition holds."
Formally, DC {CYAN}φ{ENDC} is a conjunction of predicates of the following form:
{CYAN}∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m){ENDC}

For example, look at this small table:
Name     Grade   Salary
Alice    3       3000
Bob      4       4000
Carol    4       4000

A possible DC here is: {CYAN}¬{{ t.Grade == s.Grade ∧ t.Salary != s.Salary }}{ENDC}

This means: "It should never happen that two people have the same grade but different salaries.",
or in other words, if two rows share the same Grade, they must share the same Salary.

Sometimes, we allow a DC to hold approximately, which means a small number of row pairs
might violate it. The measure used for that is the 'g1' metric. Roughly, the 'g1' metric
checks what fraction of all row pairs violates the DC, and if that fraction is lower than
a chosen threshold, we consider the DC 'valid enough.'
""")

    print(f"""{YELLOW}Mining Denial Constraints{ENDC}
We have two parameters in Desbordante's DC mining algorithm:
1) evidence_threshold: This sets the fraction of row pairs that must satisfy the DC
   for it to be considered valid. A value of 0 means exact DC mining (no violations allowed).
2) shard_length: This splits the dataset into row "shards" for parallelization.
   A value of 0 means no split, so the entire dataset is processed at once.

{YELLOW}Let's begin by looking at TABLE_1:{ENDC}""")

    print_table(TABLE_1, "TABLE_1 (examples/datasets/taxes_1.csv):")

    print(f"""{YELLOW}Mining exact DCs (evidence_threshold=0) on TABLE_1{ENDC}""")

    # Exact DC mining on TABLE_1
    algo = db.dc.algorithms.Default()
    algo.load_data(table=(TABLE_1, ',', True))
    algo.execute(evidence_threshold=0, shard_length=0)
    dcs_table1_exact = algo.get_dcs()

    print(f"{YELLOW}Discovered DCs:{ENDC}")
    for dc in dcs_table1_exact:
        print(f"  {CYAN}{dc}{ENDC}")
    print()

    print(f"""Note the following Denial Constraint we found:
{CYAN}¬{{ t.State == s.State ∧ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }}{ENDC}.
It states that for all people in the same state, the person with a higher salary
should have a higher tax rate. No pairs of rows should violate that rule.

Now let's mine approximate DCs by setting evidence_threshold to 0.5.
This means we only require that at least half of all row pairs satisfy each DC (according to 'g1').
""")

    print(f"""{YELLOW}Mining ADCs (evidence_threshold=0.5) on TABLE_1{ENDC}""")

    # Approximate DC mining on TABLE_1
    algo = db.dc.algorithms.Default()
    algo.load_data(table=(TABLE_1, ',', True))
    algo.execute(evidence_threshold=0.5, shard_length=0)
    dcs_table1_approx = algo.get_dcs()

    print(f"{YELLOW}Discovered ADCs:{ENDC}")
    for dc in dcs_table1_approx:
        print(f"  {CYAN}{dc}{ENDC}")
    print()

    print(f"""Here, for example, the 'g1' metric values for a few approximate DCs are:
{CYAN}¬{{ t.Salary <= s.Salary ∧ t.FedTaxRate <= s.FedTaxRate }}{ENDC} → 0.486111
{CYAN}¬{{ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }}{ENDC} → 0.458333
{CYAN}¬{{ t.State == s.State }}{ENDC} → 0.25
Note: A smaller 'g1' value means fewer violations, making the DC more exact.
""")

    print(f"""{YELLOW}Conclusion:{ENDC}
We found both exact and approximate DCs.

- Exact DCs are those with zero violations, so they must hold for every pair of rows.
- Approximate DCs allow some fraction of violating pairs.

Therefore, an approximate DC can logically imply the exact one.
For example, consider:
Exact DC: {CYAN}¬{{ t.State == s.State ∧ t.Salary == s.Salary }}{ENDC}
Approximate DC: {CYAN}¬{{ t.Salary == s.Salary }}{ENDC}

If the approximate DC (which prohibits any two rows from having the same Salary)
is satisfied for at least the chosen threshold, then clearly no two rows can share both
the same State and the same Salary. Thus, the approximate DC implies the exact DC.

In real scenarios, exact DCs may be too rigid.
Allowing a small fraction of violations is often a practical compromise,
but setting a very high threshold quickly becomes meaningless
since it would permit too many inconsistencies.
The best threshold often depends on how 'dirty' the data is; datasets with
more inconsistencies may require a higher threshold to capture meaningful DCs.
""")

    print(f"""{YELLOW}Now let's move on to TABLE_2{ENDC}""")

    print_table(TABLE_2, "TABLE_2 (examples/datasets/taxes_2.csv):")

    print(f"""We added this record for Texas:
{GREEN}(State=Texas, Salary=5000, FedTaxRate=0.05){ENDC}
Notice how it introduces a scenario that breaks the DC we discuissed earlier, stating
"the person with a higher salary should have a higher tax rate,"
because there are now people in Texas with a lower salary but a higher tax rate.

Let's see how the exact DC mining changes due to this additional record.
""")

    print(f"""{YELLOW}Mining exact DCs (evidence_threshold=0) on TABLE_2{ENDC}""")

    # Exact DC mining on TABLE_2
    algo = db.dc.algorithms.Default()
    algo.load_data(table=(TABLE_2, ',', True))
    algo.execute(evidence_threshold=0, shard_length=0)
    dcs_table2_exact = algo.get_dcs()

    print(f"{YELLOW}Discovered DCs:{ENDC}")
    for dc in dcs_table2_exact:
        print(f"  {CYAN}{dc}{ENDC}")
    print()

    print(f"""We can see that the DC {CYAN}¬{{ t.State == s.State ∧ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }}{ENDC}
no longer appears because of the violation introduced by record index 9
({GREEN}(Texas, 5000, 0.05){ENDC}).

Those violations occur in pairs like {RED}(6, 9), (7, 9), (8, 9){ENDC},
where each number is a record index in the dataset.""")

if __name__ == "__main__":
    main()

