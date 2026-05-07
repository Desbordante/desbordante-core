from os import path
from tracemalloc import start
import desbordante as db
import pandas as pd

print("""This example demonstrates dynamic Denial Constraint (DC) verification using the Weever algorithm.

Weever efficiently maintains the set of DC violations as rows are inserted, deleted, or updated,
avoiding a full re-verification after each change.

Reference: the algorithm is described in the Weever paper on incremental DC verification.
""")

RED_CODE = "\033[1;41m"
GREEN_CODE = "\033[1;42m"
BOLD_CODE = "\033[1;49m"
DEFAULT_COLOR_CODE = "\033[0m"

# The DC: for any two people in the same state, the one with a lower salary
# must not have a higher tax rate.
DC = "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)"

TABLE = "datasets/taxes_1.csv"


def print_table(df: pd.DataFrame, isIndexed: bool = True, startIndex: int = 0) -> None:
    display = df.copy()
    display.index = display.index + startIndex
    print(display.to_string(index=isIndexed))
    print()


def print_violations(violations: list) -> None:
    if not violations:
        print(GREEN_CODE + "DC holds — no violations found." + DEFAULT_COLOR_CODE)
    else:
        viol_str = ", ".join(f"({a}, {b})" for a, b in violations)
        print(RED_CODE + f"DC violated — violation pairs: {viol_str}" + DEFAULT_COLOR_CODE)
    print()


print(f"""DC: {DC}

The constraint says: for every pair of employees in the same state,
if one earns less they must not pay a higher tax rate.

Starting dataset (taxes_1.csv):
""")

data = pd.read_csv(TABLE)
print_table(data, True, 2)

# ── Initial verification ────────────────────────────────────────────────────
print("Step 1: initial DC verification on the base table.")

algo = db.weever.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))
algo.execute(denial_constraint=DC)

print_violations(algo.get_violations())

# ── Insert a violating row ──────────────────────────────────────────────────
print("""Step 2: insert a new Texas employee who earns 5000 but pays only 0.05 tax.
This creates a violation: Texas/5000/0.05 paired with Texas/1000/0.15
(lower salary but higher tax rate).
""")

insert_df = pd.DataFrame({"State": ["Texas"], "Salary": [5000], "FedTaxRate": [0.05]})
print("Inserted row:")
print_table(insert_df, True, 11)

algo.execute(insert=insert_df)
print_violations(algo.get_violations())

# ── Delete the violating row ────────────────────────────────────────────────
# After the insert, the new row got index 11.
# Weever tracks tuple IDs that match the index_offset used internally;
# the index used here matches what was passed to insert statements.
print("""Step 3: delete the violating row (index 11) to restore DC compliance.
""")

# Weever uses 1-based IDs counting from line 1 of the CSV (including header),
# so pandas 0-based index N maps to Weever ID N+2.
violating_index = data.index.max() + 3  # = 11

algo.execute(denial_constraint=DC, delete={violating_index})
print_violations(algo.get_violations())
