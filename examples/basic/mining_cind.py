import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

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


def short_attr(attr):
    return attr.replace(".csv", "").replace("cind_test_", "")


def fmt_attrs(attrs):
    cleaned = [short_attr(a) for a in attrs]
    if all("." in a for a in cleaned):
        tables = {a.split(".", 1)[0] for a in cleaned}
        if len(tables) == 1:
            t = tables.pop()
            cols = [a.split(".", 1)[1] for a in cleaned]
            return f"{t}.[{', '.join(cols)}]"
    return ", ".join(cleaned)


def mine(error, validity, completeness, condition_type, algo_type="pli_cind"):
    algo = desbordante.cind.algorithms.Default()
    algo.load_data(tables=TABLES)
    algo.execute(error=error, validity=validity, completeness=completeness,
                 condition_type=condition_type, algo_type=algo_type)
    return algo.get_cinds()


banner("Discovering Conditional Inclusion Dependencies (CINDs)")

print("References:")
print("  [1] L. Bravo, W. Fan, S. Ma. Extending Dependencies with Conditions.")
print("      VLDB 2007, pp. 243-254.  -- introduces CINDs.")
print("  [2] J. Bauckmann, Z. Abedjan, U. Leser, H. Muller, F. Naumann.")
print("      Discovering Conditional Inclusion Dependencies. CIKM 2012,")
print("      pp. 2094-2098.  -- the CINDERELLA and PLI-CIND algorithms used here.")
print("  [3] O. Cure. Improving the Data Quality of Drug Databases using")
print("      Conditional Dependencies and Ontologies. ACM JDIQ 4(1):20, 2012.")
print("      -- CIND violation detection in a data-cleaning setting.")
print()

print("Related primitives in Desbordante:")
print("  * CIND verification  -  examples/basic/verifying_cind.py")
print("  * plain IND/AIND     -  examples/basic/verifying_ind_aind.py")
print("  * CFD                -  examples/basic/verifying_cfd.py")
print()

print(f"{YELLOW}>>> Definition 1 (IND).{RESET}")
prints(
    "An Inclusion Dependency R1[X] subseteq R2[Y] holds when every combination of "
    "values in the columns X of R1 also appears in columns Y of R2."
)
print()

print(f"{YELLOW}>>> Definition 2 (CIND).{RESET}")
prints(
    "A Conditional Inclusion Dependency restricts an IND to a subset of R1 "
    "rows specified by a pattern over the remaining columns of R1 (the "
    "conditional attributes). A pattern entry is either a concrete value or "
    "a wildcard. This example demonstrates the CIND mining algorithm, which "
    "discovers CINDs that hold on a dataset."
)
print()


print(f"{CYAN}Datasets{RESET}")
print("-" * 80)

printlns(
    "Two toy tables, en and de, with the same people taken from the English "
    "and German editions of Wikipedia. Columns: pid, cent (century), "
    "birthplace, deathplace, desc."
)

en = pd.read_csv("examples/datasets/ind_datasets/cind_test_en.csv")
de = pd.read_csv("examples/datasets/ind_datasets/cind_test_de.csv")
print_table(en, "en:")
print_table(de, "de:")

TABLES = [(f"examples/datasets/ind_datasets/{name}.csv", ",", True)
          for name in ["cind_test_de", "cind_test_en"]]


print(f"{CYAN}Algorithm parameters{RESET}")
print("-" * 80)

print("  * error          AIND error threshold; the IND finder accepts INDs")
print("                   whose error is at most this value.")
print()
print("  * validity       lower bound on the covering condition score")
print("                   (precision-like: how correct the CIND is).")
print()
print("  * completeness   lower bound on the completeness score")
print("                   (recall-like: how much of the data the condition covers).")
print()
print("  * condition_type \"row\"   - statistics aggregated per individual row;")
print("                   \"group\" - per basket (rows sharing the inclusion key).")
print()
print("  * algo_type      \"pli_cind\" (default, depth-first, low memory) or")
print("                   \"cinderella\" (breadth-first, faster but heavier).")
print()


banner("Scenario 1. Mine CINDs with relaxed thresholds")

printlns(
    "The thresholds below are deliberately loose so the algorithm returns "
    "many candidates: error=0.5, validity=0.75, completeness=0.25. "
    "Scenario 3 uses stricter values for comparison."
)

cinds = mine(error=0.5, validity=0.75, completeness=0.25, condition_type="row")
print(f"  found {len(cinds)} CIND(s):")
for i, c in enumerate(cinds, start=1):
    print(f"    #{i:<2}  {fmt_attrs(c.get_condition_attributes()):<48} "
          f"{c.conditions_number():>3} cond.")
print()

if not cinds:
    printlns(
        "No CINDs were found. Lower validity or completeness, or raise error, "
        "to widen the search."
    )


banner("Scenario 2. Inspecting CIND and Condition objects")

printlns(
    "Each item returned by get_cinds() is a CIND object, which carries its "
    "conditional attributes and a list of Condition objects. A Condition "
    "stores the concrete pattern values together with the validity and "
    "completeness measured for that pattern."
)

if cinds:
    cind = cinds[0]
    print(f"  condition_attributes:  {fmt_attrs(cind.get_condition_attributes())}")
    print(f"  conditions_number:     {cind.conditions_number()}")
    print()

    conditions = cind.get_conditions()
    if conditions:
        cond = conditions[0]
        print(f"  first Condition.data():         {cond.data()}")
        print(f"                  validity():     {cond.validity():.3f}")
        print(f"                  completeness(): {cond.completeness():.3f}")
    else:
        printlns("This CIND has no conditions.")
    print()

    printlns(
        "CIND and Condition both implement __str__, __eq__ and __hash__, so "
        "they are usable as dictionary keys and inside sets."
    )
else:
    printlns("Skipping object inspection because Scenario 1 returned no CINDs.")


banner("Scenario 3. Tighter thresholds and a different condition type")

printlns(
    "Raising the validity threshold and switching to condition_type=\"group\" "
    "asks the algorithm for stricter, basket-level patterns. The result is "
    "fewer but more reliable CINDs."
)

cinds_strict = mine(error=0.3, validity=0.95, completeness=0.5,
                    condition_type="group")
print(f"  found {len(cinds_strict)} CIND(s):")
for i, c in enumerate(cinds_strict, start=1):
    print(f"    #{i:<2}  {fmt_attrs(c.get_condition_attributes()):<48} "
          f"{c.conditions_number():>3} cond.")
print()

printlns(
    "In practice the workflow is iterative: start permissive to see what is "
    "in the data, then tighten thresholds and switch row/group to focus on "
    "the patterns that matter."
)


banner("See also")

printlns(
    "examples/basic/verifying_cind.py - validates a given CIND on data and "
    "reports the rows that violate it."
)
