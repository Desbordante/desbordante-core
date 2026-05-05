import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

GREEN = "\033[1;42m"
RED = "\033[1;41m"
BLUE = "\033[1;46m"
YELLOW = "\033[1;33m"
CYAN = "\033[1;36m"
RESET = "\033[0m"


def prints(s):
    print(textwrap.fill(s, 80))


def printlns(s):
    prints(s)
    print()


def banner(title):
    print("=" * 80)
    print(f"{CYAN}{title}{RESET}")
    print("=" * 80)


def print_table(df, title):
    print(title)
    print(tabulate(df, headers="keys", tablefmt="psql"))
    print()


def verify(lhs, rhs, lhs_indices, rhs_indices, cond_vals=None):
    algo = desbordante.cind_verification.algorithms.Default()
    algo.load_data(tables=[lhs, rhs])
    kwargs = {"lhs_indices": lhs_indices, "rhs_indices": rhs_indices,
              "condition_type": "group"}
    if cond_vals:
        kwargs["cind_condition_values"] = cond_vals
    algo.execute(**kwargs)
    return algo


def print_result(algo):
    tag = f"{GREEN} holds {RESET}" if algo.holds() else f"{RED} does not hold {RESET}"
    print(f"  {tag}  validity = {algo.get_real_validity():.3f}   "
          f"completeness = {algo.get_real_completeness():.3f}")


def print_violations(algo, df, indices):
    n = algo.get_violating_clusters_count()
    if n == 0:
        return
    print(f"  {n} violating cluster(s):")
    for i, cl in enumerate(algo.get_violating_clusters(), start=1):
        print(f"  {BLUE} #{i} {RESET}  basket_rows = {list(cl.basket_rows)}  "
              f"violating_rows = {list(cl.violating_rows)}")
        for rid in cl.violating_rows:
            vals = " ".join(str(df.iloc[rid, idx]) for idx in indices)
            print(f"      row {rid}: {vals}")
    print()


banner("Verifying Conditional Inclusion Dependencies (CINDs)")

printlns(
    "In this example we consider a Conditional Inclusion Dependency, check "
    "whether it holds on a small dataset, measure how close to holding it "
    "is, and inspect the rows that violate it. Definitions follow [1, 2]; "
)

print(f"{YELLOW}>>> Definition 1 (IND) [1].{RESET}")
prints(
    "An Inclusion Dependency R1[X] subseteq R2[Y] holds when every combination of "
    "values in the columns X of R1 also appears in columns Y of R2. R1 with "
    "its columns X is called the left-hand side (LHS, the dependent table); "
    "R2 with its columns Y is called the right-hand side (RHS, the "
    "referenced table)."
)
print()

print(f"{YELLOW}>>> Definition 2 (CIND) [1, 2].{RESET}")
prints(
    "A Conditional Inclusion Dependency extends an IND by restricting the "
    "rule to R1 rows that match a pattern on the remaining columns of R1 "
    "(the conditional attributes). Pattern entries are either concrete "
    "values or a wildcard (\"_\" or \"-\") meaning any value. An "
    "all-wildcard pattern reduces a CIND to a plain IND."
)
print()


print(f"{CYAN}Datasets{RESET}")
print("-" * 80)

printlns(
    "Two toy tables, en and de, with the same people taken from the English "
    "and German editions of Wikipedia. Columns: pid, cent (century), "
    "birthplace, deathplace, desc (description, profession)."
)

en = pd.read_csv("examples/datasets/ind_datasets/cind_test_en.csv")
de = pd.read_csv("examples/datasets/ind_datasets/cind_test_de.csv")
print_table(en, "en:")
print_table(de, "de:")

printlns(
    "The example verifies CINDs of the form en[pid] subseteq de[pid] with an "
    "optional condition over the four remaining columns of en. The condition "
    "is passed as cind_condition_values - a list of length four, aligned "
    "with the en column order."
)


banner("Scenario 1. Empty condition: CIND reduces to IND")

printlns("Without a pattern every pid from en table is required to appear in de.")

algo = verify(en, de, lhs_indices=[0], rhs_indices=[0])
print_result(algo)
print()

print("The two metrics answer two different questions:")
print("  * validity     = |matching LHS rows included in RHS| / |matching LHS rows|")
print("                   precision-like; is the CIND correct on the selected rows?")
print("  * completeness = |matching LHS rows included in RHS| / |all included LHS rows|")
print("                   recall-like; does the condition cover all included rows?")
print()

printlns(
    "Violations are reported as clusters. A cluster groups LHS rows that "
    "share the same inclusion key; basket_rows lists every row in the "
    "cluster; violating_rows lists the offenders (rows that match the "
    "condition but whose key is missing from the RHS)."
)
print_violations(algo, en, [0])


banner("Scenario 2. basket_rows vs violating_rows")

printlns(
    "In Scenario 1 the included attribute of the left table (en.pid) contained"
    " no duplicates, so basket_rows and violating_rows were identical."
    " They diverge when a single inclusion key has several LHS rows,"
    " only some of which match the condition. To show this we add"
    " a second row about Buddy Roosevelt to en."
)

en_dup = pd.concat([en, pd.DataFrame([{
    "pid": "Buddy Roosevelt", "cent": 18, "birthplace": "NY",
    "deathplace": "NY", "desc": "Actor",
}])], ignore_index=True)
print_table(en_dup, "en (extended):")

algo = verify(en_dup, de, [0], [0], cond_vals=["_", "_", "_", "Actor"])
print("  CIND: en[pid] subseteq de[pid] | (desc = 'Actor')")
print_result(algo)
print_violations(algo, en_dup, [0])

printlns(
    "The Buddy basket now has two rows. Only the new one (row 4) matches "
    "desc='Actor', so violating_rows holds just that single row. basket_rows "
    "still lists every row of the cluster - useful context when deciding "
    "whether to fix the data or refine the condition."
)

printlns(
    "Both metrics also drop to 0.5. Plugging this scenario into the formulas:"
)
print("  |matching LHS rows|                 = 2   { Cecil, Buddy }")
print("  |matching LHS rows included in RHS| = 1   { Cecil }")
print("  |all included LHS rows|             = 2   { Cecil, Mel Sheppard }")
print()
print("  validity     = 1 / 2 = 0.500")
print("  completeness = 1 / 2 = 0.500")
print()
printlns(
    "Buddy is the false positive (matched the condition but missing in de); "
    "Mel is the recall miss (in de but not picked up by desc='Actor', they "
    "are an athlete)."
)


banner("Scenario 3. A condition the data already satisfies")

printlns("Let's add the condition desc = 'Actor' and put wildcards in the other positions.")

algo = verify(en, de, [0], [0], cond_vals=["_", "_", "_", "Actor"])
print("  CIND: en[pid] subseteq de[pid] | (desc = 'Actor')")
print_result(algo)
print()

printlns(
    "Only Cecil Kellaway passes the filter, and they are in de, so the CIND "
    "holds exactly."
)


banner("Scenario 4. Partial validity, then a data fix")

printlns(
    "Conditioning on cent = 18 selects three en table rows, but only two of "
    "their pids are in de."
)

algo = verify(en, de, [0], [0], cond_vals=["18", "_", "_", "_"])
print("  CIND: en[pid] subseteq de[pid] | (cent = '18')")
print_result(algo)
print_violations(algo, en, [0])

printlns("The cluster points at Buddy Roosevelt: present in en, absent from de.")

print("-" * 80)
print()

printlns(
    "Now, a harder case: desc = 'Olympic' selects Sante Gaiardoni, who is not in "
    "de at all."
)

cond_olympic = ["_", "_", "_", "Olympic"]
algo = verify(en, de, [0], [0], cond_vals=cond_olympic)
print("  CIND: en[pid] subseteq de[pid] | (desc = 'Olympic')")
print_result(algo)
print_violations(algo, en, [0])

printlns("Let's treat it as a gap in de and fix data by adding the missing entry.")

de_fixed = pd.concat([de, pd.DataFrame([{
    "pid": "Sante Gaiardoni", "cent": 19, "birthplace": "-",
    "deathplace": "-", "desc": "Olympionike",
}])], ignore_index=True)
print_table(de_fixed, "de (patched):")

algo = verify(en, de_fixed, [0], [0], cond_vals=cond_olympic)
print("  CIND: en[pid] subseteq de[pid] | (desc = 'Olympic')")
print_result(algo)
print()

printlns(
    "The CIND now holds. A typical workflow is: verify, read the violating "
    "clusters, adjust data or condition, verify again."
)


banner("Scenario 5. Wrong number of condition values")

printlns(
    "cind_condition_values must have one entry per conditional attribute "
    "(four, for this dataset). A wrong length raises an exception at "
    "execute() time as shown below."
)

try:
    bad = desbordante.cind_verification.algorithms.Default()
    bad.load_data(tables=[en, de])
    bad.execute(lhs_indices=[0], rhs_indices=[0],
                cind_condition_values=["18", "_"])
except Exception as e:
    print(f"  caught: {e}")
print()


banner("See also")

print("Related primitives in Desbordante:")
print("  * CIND mining           -  examples/basic/mining_cind.py")
print("  * IND mining            -  examples/basic/mining_ind.py")
print("  * AIND mining           -  examples/basic/mining_aind.py")
print("  * IND/AIND verification -  examples/basic/verifying_ind_aind.py")
print("  * CFD                   -  examples/basic/verifying_cfd.py")
print()

print("References:")
print("  [1] L. Bravo, W. Fan, S. Ma. Extending Dependencies with Conditions.")
print("      VLDB 2007, pp. 243-254.  -- introduces CINDs.")
print("  [2] O. Cure. Improving the Data Quality of Drug Databases using")
print("      Conditional Dependencies and Ontologies. ACM JDIQ 4(1):20, 2012.")
print("      -- CIND violation detection in a data-cleaning setting.")
print("  [3] J. Bauckmann, Z. Abedjan, U. Leser, H. Muller, F. Naumann.")
print("      Discovering Conditional Inclusion Dependencies. CIKM 2012,")
print("      pp. 2094-2098.  -- CINDERELLA and PLI-CIND.")
print()
