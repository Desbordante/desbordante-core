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


def fmt_ind(ind_string):
    import re
    m = re.match(
        r"\(([^,]+),\s*\[([^\]]*)\]\)\s*->\s*\(([^,]+),\s*\[([^\]]*)\]\)",
        ind_string,
    )
    if not m:
        return ind_string
    lhs_t, lhs_c, rhs_t, rhs_c = m.groups()
    lhs_t = lhs_t.replace("cind_test_", "").replace(".csv", "")
    rhs_t = rhs_t.replace("cind_test_", "").replace(".csv", "")
    return f"{lhs_t}.[{lhs_c}] subseteq {rhs_t}.[{rhs_c}]"


def mine(error, validity, completeness, condition_type, algo_type="pli_cind"):
    algo = desbordante.cind.algorithms.Default()
    algo.load_data(tables=TABLES)
    algo.execute(error=error, validity=validity, completeness=completeness,
                 condition_type=condition_type, algo_type=algo_type)
    return algo, algo.get_cinds()


banner("Discovering Conditional Inclusion Dependencies (CINDs)")

printlns(
    "In this example we discover Conditional Inclusion Dependencies on a "
    "small dataset, set thresholds that filter the search, and inspect the "
    "patterns the algorithm finds. Definitions follow [1, 2]; full citations "
    "are at the bottom."
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
    "values or a wildcard (\"_\" or \"-\") meaning any value."
)
print()

print(f"{YELLOW}>>> Mining pipeline.{RESET}")
prints(
    "Mining works in two stages. Spider first searches for Approximate "
    "Inclusion Dependencies (AINDs) - inclusions that hold up to a small "
    "fraction of mismatches, controlled by the error threshold. AIND "
    "mining as a standalone primitive is shown in "
    "examples/basic/mining_aind.py. Then, for each AIND, a condition miner "
    "narrows it down to concrete patterns. This example uses the algorithms "
    "from [2]: CINDERELLA (breadth-first) and PLI-CIND (depth-first); both "
    "produce the same CINDs but trade off speed against memory."
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

TABLES = [(f"examples/datasets/ind_datasets/{name}.csv", ",", True)
          for name in ["cind_test_de", "cind_test_en"]]


print(f"{CYAN}Metrics{RESET}")
print("-" * 80)

print("Each candidate pattern is scored by two metrics:")
print("  * validity     = |matching LHS rows included in RHS| / |matching LHS rows|")
print("                   precision-like; how correct the CIND is on the selected rows.")
print("  * completeness = |matching LHS rows included in RHS| / |all included LHS rows|")
print("                   recall-like; how much of the included data the condition covers.")
print()
prints(
    "Worked example. Plug the candidate CIND "
    "en[pid] subseteq de[pid] | desc='Actor' into the formulas above:"
)
print()
print("  |matching LHS rows|                 = 1   { Cecil Kellaway }")
print("  |matching LHS rows included in RHS| = 1   { Cecil Kellaway }")
print("  |all included LHS rows|             = 2   { Cecil, Mel Sheppard }")
print()
print("  validity     = 1 / 1 = 1.000")
print("  completeness = 1 / 2 = 0.500")
print()
prints(
    "The mining algorithm keeps only patterns that meet user-supplied lower "
    "bounds on both metrics."
)
print()


print(f"{CYAN}condition_type: row vs group{RESET}")
print("-" * 80)

prints(
    "The condition_type parameter chooses the unit of counting. In \"group\" "
    "mode all LHS rows that share the same inclusion key are counted "
    "together as one, regardless of how many rows there are. In \"row\" "
    "mode every LHS row counts on its own. On unique-key tables the two "
    "modes coincide; where the same key recurs, \"group\" stays robust and "
    "\"row\" tends to inflate totals through duplicates."
)
print()
prints(
    "This parameter directly affects validity and completeness calculation."
)
print()


print(f"{CYAN}Algorithm parameters{RESET}")
print("-" * 80)

print("  * error          AIND error threshold; the IND finder accepts INDs")
print("                   whose error is at most this value.")
print()
print("  * validity       lower bound on the validity score (see Metrics).")
print()
print("  * completeness   lower bound on the completeness score (see Metrics).")
print()
print("  * condition_type \"row\" or \"group\" (see the section above).")
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

algo1, cinds = mine(error=0.5, validity=0.75, completeness=0.25, condition_type="row")
print(f"  found {len(cinds)} CIND(s):")
for i, c in enumerate(cinds, start=1):
    print(f"    #{i:<2}  {fmt_ind(c.get_ind_string()):<48} "
          f"{c.conditions_number():>3} cond.")
print()

if not cinds:
    printlns(
        "No CINDs were found. Lower validity or completeness, or raise error, "
        "to widen the search."
    )
else:
    printlns(
        "Each line shows the underlying inclusion dependency the CIND "
        "refines (LHS table and columns subseteq RHS table and columns) "
        "and the number of concrete patterns the algorithm found over the "
        "remaining LHS columns - the conditional attributes."
    )

    sample = cinds[0]
    sample_conds = sample.get_conditions()
    sample_attrs = fmt_attrs(sample.get_condition_attributes())
    if sample_conds:
        print(f"First few conditions of CIND #1 ({fmt_ind(sample.get_ind_string())}):")
        print(f"  conditional attributes (in column order): {sample_attrs}")
        print()
        for j, cond in enumerate(sample_conds[:3], start=1):
            print(f"  {j}. data = {cond.data()},  validity = {cond.validity():.3f},"
                  f"  completeness = {cond.completeness():.3f}")
        if len(sample_conds) > 3:
            print(f"  ... ({len(sample_conds) - 3} more)")
        print()
        printlns(
            "Each Condition lists concrete values for the conditional "
            "attributes (in column order); '-' means a wildcard. validity "
            "and completeness are measured for that specific pattern."
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
    print(f"  IND:                   {fmt_ind(cind.get_ind_string())}")
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
    "Tighter thresholds: error=0.3, validity=0.95, completeness=0.5, "
    "condition_type=\"group\". The result is fewer but more reliable CINDs; "
    "some come back with 0 conditions because the underlying AIND exists "
    "but no concrete pattern passed validity >= 0.95 and completeness >= 0.5."
)

algo2, cinds_strict = mine(error=0.3, validity=0.95, completeness=0.5,
                           condition_type="group")
print(f"  found {len(cinds_strict)} CIND(s):")
for i, c in enumerate(cinds_strict, start=1):
    print(f"    #{i:<2}  {fmt_ind(c.get_ind_string()):<48} "
          f"{c.conditions_number():>3} cond.")
print()

printlns(
    "In practice the workflow is iterative: start permissive to see what is "
    "in the data, then tighten thresholds and switch row/group to focus on "
    "the patterns that matter."
)


banner("See also")

print("Related primitives in Desbordante:")
print("  * CIND verification     -  examples/basic/verifying_cind.py")
print("  * IND mining            -  examples/basic/mining_ind.py")
print("  * AIND mining           -  examples/basic/mining_aind.py")
print("  * IND/AIND verification -  examples/basic/verifying_ind_aind.py")
print("  * CFD                   -  examples/basic/verifying_cfd.py")
print()

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
