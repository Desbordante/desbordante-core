import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"
RESET_CODE = "\033[0m"

print("""==============================================
In Desbordante, a Conditional Inclusion Dependency (CIND) is an inclusion
dependency that is checked only on those LHS tuples that satisfy a condition
on the *other* (conditional) attributes of the LHS table.

Informally:
    (LHS_inclusion_attributes) in (RHS_inclusion_attributes)   |   CONDITION(on LHS-only attrs)

Key points for this verifier:
- You specify lhs_indices and rhs_indices (same arity).
- Conditional attributes are all columns of the LHS table that are NOT in lhs_indices
  (and if you verify within one table, also NOT in rhs_indices).
- The option `cind_condition_values` must be aligned with the conditional attributes order
  (i.e., in the LHS table column order after filtering as described above).
- Wildcards: "-", "_" (and also the library wildcard constant) mean "any value".

The verifier returns:
- real_validity   = included_support / supporting_baskets
- real_completeness = included_support / included_baskets_total

=============================================="""
)

def prints(s: str):
    print(textwrap.fill(s, 80))

def print_table(df: pd.DataFrame, title: str):
    print(title)
    print(tabulate(df, headers="keys", tablefmt="psql"), end="\n\n")

def get_table_df(dataset_name: str) -> pd.DataFrame:
    return pd.read_csv(f"examples/datasets/ind_datasets/{dataset_name}.csv", header=[0])

def safe_get(obj, *names, default=None):
    for n in names:
        if hasattr(obj, n):
            return getattr(obj, n)()
    return default

def cind_str(lhs, rhs, cond_cols, cond_vals):
    (lhs_name, lhs_df, lhs_indices) = lhs
    (rhs_name, rhs_df, rhs_indices) = rhs
    lhs_cols = ", ".join(f"{lhs_name}.{lhs_df.columns[i]}" for i in lhs_indices)
    rhs_cols = ", ".join(f"{rhs_name}.{rhs_df.columns[i]}" for i in rhs_indices)
    cond_parts = []
    for c, v in zip(cond_cols, cond_vals):
        if v in ("-", "_"):
            cond_parts.append(f"{lhs_name}.{c}=*")
        else:
            cond_parts.append(f"{lhs_name}.{c}={v}")
    cond_str = ", ".join(cond_parts) if cond_parts else "*"
    return f"[{lhs_cols}] in [{rhs_cols}]  |  ({cond_str})"

def get_conditional_columns(lhs_df: pd.DataFrame, lhs_indices, rhs_indices, same_table: bool):
    lhs_set = set(lhs_indices)
    rhs_set = set(rhs_indices)
    cols = []
    for i, col in enumerate(lhs_df.columns):
        if i in lhs_set:
            continue
        if same_table and i in rhs_set:
            continue
        cols.append(col)
    return cols

def print_results(verifier):
    validity = verifier.get_real_validity()
    completeness = verifier.get_real_completeness()

    holds = safe_get(verifier, "holds", "Holds", default=None)
    if holds is None:
        holds = (validity == 1.0)

    if holds:
        print(GREEN_CODE, f"CIND holds (validity={validity:.3f}, completeness={completeness:.3f})",
              RESET_CODE)
    else:
        print(RED_CODE, f"CIND does NOT hold (validity={validity:.3f}, completeness={completeness:.3f})",
              RESET_CODE)

def normalize_cluster(cluster):
    if isinstance(cluster, dict):
        basket = cluster.get("basket_rows", cluster.get("basket", []))
        viol = cluster.get("violating_rows", cluster.get("violations", []))
        return list(basket), list(viol)

    if hasattr(cluster, "basket_rows") and hasattr(cluster, "violating_rows"):
        return list(cluster.basket_rows), list(cluster.violating_rows)

    if isinstance(cluster, (list, tuple)) and len(cluster) == 2:
        a, b = cluster
        if isinstance(a, (list, tuple)) and isinstance(b, (list, tuple)):
            return list(a), list(b)

    if isinstance(cluster, (list, tuple)):
        return list(cluster), list(cluster)

    return [], []

def print_clusters(verifier, lhs_df: pd.DataFrame, lhs_indices):
    cnt = verifier.get_violating_clusters_count()
    print(f"Number of violating clusters: {cnt}")

    clusters = verifier.get_violating_clusters()
    for i, cl in enumerate(clusters, start=1):
        basket_rows, violating_rows = normalize_cluster(cl)

        print(f"{BLUE_CODE} #{i} cluster {DEFAULT_COLOR_CODE}")
        print(f"basket_rows={basket_rows}")
        print(f"violating_rows={violating_rows}")

        for rid in violating_rows:
            values = " ".join(str(lhs_df.iloc[rid, idx]) for idx in lhs_indices)
            print(f"  row {rid}: {values}")
        print()

    print(RESET_CODE)

def cind_verify(lhs, rhs, cond_vals, condition_type="group"):
    (lhs_name, lhs_df, lhs_indices) = lhs
    (rhs_name, rhs_df, rhs_indices) = rhs

    print_table(lhs_df, f"Dataset '{lhs_name}':")
    print_table(rhs_df, f"Dataset '{rhs_name}':")

    same_table = (lhs_name == rhs_name)
    cond_cols = get_conditional_columns(lhs_df, lhs_indices, rhs_indices, same_table)
    dep_str = cind_str(lhs, rhs, cond_cols, cond_vals)
    print(f"Checking CIND: {dep_str}")

    algo = desbordante.cind_verification.algorithms.Default()
    algo.load_data(tables=[lhs_df, rhs_df])

    kwargs = dict(
        lhs_indices=lhs_indices,
        rhs_indices=rhs_indices,
        condition_type=condition_type,
    )
    if cond_vals is not None and len(cond_vals) > 0:
        kwargs["cind_condition_values"] = cond_vals

    algo.execute(**kwargs)
    return algo, cond_cols

def scenario_unconditional_ind_like():
    prints("Scenario 1: empty condition -> CIND behaves like IND on the inclusion attributes.")
    print()

    lhs_df = get_table_df("cind_test_en")
    rhs_df = get_table_df("cind_test_de")

    lhs = ("en", lhs_df, [0])
    rhs = ("de", rhs_df, [0])

    algo, cond_cols = cind_verify(lhs, rhs, cond_vals=[])
    print_results(algo)
    print()

    prints("Because the condition is empty (all wildcards), the verifier checks inclusion for all "
           "distinct LHS values, exactly like IND.")
    print()
    print_clusters(algo, lhs_df, [0])

def scenario_condition_makes_it_hold():
    prints("Scenario 2: add a condition to focus only on a specific subset of LHS tuples.")
    print()

    lhs_df = get_table_df("cind_test_en")
    rhs_df = get_table_df("cind_test_de")

    lhs = ("en", lhs_df, [0])
    rhs = ("de", rhs_df, [0])

    cond_cols = get_conditional_columns(lhs_df, [0], [0], same_table=False)
    prints(f"Conditional attributes order for this dataset: {', '.join(cond_cols)}")
    prints("So `cind_condition_values` must be a list of the same length in exactly this order.")
    print()

    cond_vals = ["_", "_", "_", "Actor"]
    algo, _ = cind_verify(lhs, rhs, cond_vals=cond_vals)
    print_results(algo)
    print()

    prints("Here we restricted the check to EN rows where desc == 'Actor'. "
           "That selects only 'Cecil Kellaway', and that pid exists in the DE table.")
    print()
    print_clusters(algo, lhs_df, [0])

def scenario_partial_validity_and_fix():
    prints("Scenario 3: a condition that selects multiple rows can yield partial validity.")
    print()

    lhs_df = get_table_df("cind_test_en")
    rhs_df = get_table_df("cind_test_de")

    lhs = ("en", lhs_df, [0])
    rhs = ("de", rhs_df, [0])

    cond_vals = ["18", "_", "_", "_"]
    algo, _ = cind_verify(lhs, rhs, cond_vals=cond_vals)
    print_results(algo)
    print()

    prints("Condition cent == 18 selects three EN rows. Two of their pids are present in DE, "
           "so validity is 2/3.")
    print()
    print_clusters(algo, lhs_df, [0])

    print("-" * 80)
    print()

    prints("Now let's make a previously failing condition hold by fixing the reference (DE) table.")
    prints("We will check for desc == 'Olympic' (this selects 'Sante Gaiardoni' in EN), "
           "then add that pid to DE.")
    print()

    cond_vals_olympic = ["_", "_", "_", "Olympic"]
    algo_bad, _ = cind_verify(lhs, rhs, cond_vals=cond_vals_olympic)
    print_results(algo_bad)
    print()
    print_clusters(algo_bad, lhs_df, [0])

    prints("Fix: add a matching row for 'Sante Gaiardoni' to the DE table.")
    print()

    new_row = {
        "pid": "Sante Gaiardoni",
        "cent": 19,
        "birthplace": "-",
        "deathplace": "-",
        "desc": "Olympionike",
    }
    rhs_df_fixed = pd.concat([rhs_df, pd.DataFrame([new_row])], ignore_index=True)

    rhs_fixed = ("de_fixed", rhs_df_fixed, [0])
    algo_fixed, _ = cind_verify(lhs, rhs_fixed, cond_vals=cond_vals_olympic)
    print_results(algo_fixed)
    print()

    prints("After adding the missing pid, the conditioned dependency becomes satisfied.")
    print()
    print_clusters(algo_fixed, lhs_df, [0])

prints("This example demonstrates CIND verification on your datasets (EN and DE).")
prints("Place these files into: examples/datasets/cind_datasets/")
prints("  - cind_test_en.csv")
prints("  - cind_test_de.csv")
print()

scenario_unconditional_ind_like()
print()
print("=" * 80)
print()
scenario_condition_makes_it_hold()
print()
print("=" * 80)
print()
scenario_partial_validity_and_fix()
