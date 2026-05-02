import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap
import pprint

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


def print_attrs_line(label, attrs_str, indent):
    pad = " " * indent
    line = f"{pad}{label}: "
    wrapped = textwrap.fill(attrs_str, width=80,
                            initial_indent=line, subsequent_indent=" " * len(line))
    print(wrapped)


def print_condition(cond, num, indent):
    pad = " " * indent
    label = f"{pad}{num}. data = "
    data_str = pprint.pformat(cond.data(), width=80 - len(label), compact=True)
    lines = data_str.splitlines()
    print(f"{label}{lines[0]}")
    cont_pad = " " * len(label)
    for line in lines[1:]:
        print(f"{cont_pad}{line}")
    print(f"{cont_pad}validity = {cond.validity():.3f}, "
          f"completeness = {cond.completeness():.3f}")


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


def mine_cure(error, support):
    algo = desbordante.cind.algorithms.Default()
    algo.load_data(tables=TABLES, algo_type="cure_cind")
    algo.execute(error=error, support=support)
    return algo, algo.get_cinds()


def mine_cinderella(error, validity, completeness, condition_type="row"):
    algo = desbordante.cind.algorithms.Default()
    algo.load_data(tables=TABLES, algo_type="cinderella")
    algo.execute(error=error, validity=validity, completeness=completeness,
                 condition_type=condition_type)
    return algo, algo.get_cinds()


banner("Discovering CINDs with the Cure algorithm")

printlns(
    "In this example we discover Conditional Inclusion Dependencies with the "
    "Cure algorithm [3], an alternative to the CINDERELLA / PLI-CIND miners "
    "shown in mining_cind.py. Cure is controlled by a single support "
    "threshold and produces compact patterns with disjunctive RHS values."
)

print(f"{YELLOW}>>> Definition (CIND) [1, 2].{RESET}")
prints(
    "A Conditional Inclusion Dependency restricts an IND R1[X] subseteq R2[Y] "
    "to R1 rows that match a pattern on the remaining columns of R1 (the "
    "conditional attributes). Pattern entries are concrete values or a "
    "wildcard (\"_\" or \"-\") meaning any value. See mining_cind.py for the "
    "full definition and the validity/completeness metrics used by other "
    "miners."
)
print()

print(f"{YELLOW}>>> Cure pipeline [3].{RESET}")
prints(
    "Mining works in two stages. Spider first searches for Approximate "
    "Inclusion Dependencies (AINDs) - inclusions that hold up to a small "
    "fraction of mismatches, controlled by the error threshold. Then, for "
    "each AIND, Cure runs in two phases:"
)
print()
print("  1. Discovery: for every pair of (LHS conditional attribute, RHS")
print("     conditional attribute), hash-join the rows on the inclusion key")
print("     and count co-occurrences of (LHS value, RHS value) pairs. Keep")
print("     pairs whose count is at least the support threshold.")
print()
print("  2. Minimal cover: merge patterns sharing the same (LHS attribute,")
print("     LHS value) into one tableau row. When several RHS values appear")
print("     for the same LHS key, they are folded into a comma-separated")
print("     disjunction in the corresponding RHS slot.")
print()
prints(
    "Validity and completeness for the resulting patterns are derived from "
    "the per-pattern support and the total number of joined tuples; they are "
    "informational only - Cure does not filter by them."
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


print(f"{CYAN}Algorithm parameters{RESET}")
print("-" * 80)

print("  * error    AIND error threshold; the IND finder accepts INDs")
print("             whose error is at most this value.")
print()
print("  * support  minimum number of joined (LHS, RHS) tuple pairs that")
print("             must back a (LHS value, RHS value) pattern for it to")
print("             be kept. Larger support -> fewer, more reliable")
print("             patterns; smaller support -> wider exploration.")
print()


banner("Scenario 1. Mine CINDs with relaxed support")

printlns(
    "Start with support=1 so any pattern observed at least once is kept. "
    "This is the broadest setting and shows what kinds of patterns Cure "
    "produces on this dataset."
)

algo1, cinds = mine_cure(error=0.5, support=1)
print(f"  found {len(cinds)} CIND(s):")
for i, c in enumerate(cinds, start=1):
    print(f"    #{i:<2}  {fmt_ind(c.get_ind_string()):<48} "
          f"{c.conditions_number():>3} cond.")
print()

if not cinds:
    printlns(
        "No CINDs were found. Lower support or raise error to widen the search."
    )
else:
    printlns(
        "Each line shows the underlying inclusion dependency the CIND "
        "refines (LHS table and columns subseteq RHS table and columns) "
        "and the number of concrete patterns Cure found over the remaining "
        "LHS columns - the conditional attributes."
    )

    sample = cinds[0]
    sample_conds = sample.get_conditions()
    sample_attrs = fmt_attrs(sample.get_condition_attributes())
    if sample_conds:
        print(f"First few conditions of CIND #1 ({fmt_ind(sample.get_ind_string())}):")
        print_attrs_line("conditional attributes (in column order)",
                         sample_attrs, indent=2)
        print()
        for j, cond in enumerate(sample_conds[:3], start=1):
            print_condition(cond, j, indent=2)
        if len(sample_conds) > 3:
            print(f"  ... ({len(sample_conds) - 3} more)")
        print()
        printlns(
            "Each Condition lists concrete values for the conditional "
            "attributes (in column order); '-' means a wildcard. A "
            "comma-separated value (\"a, b\") is a disjunction produced by "
            "the minimal-cover phase: the same LHS key matched several RHS "
            "values, all of which satisfy the pattern."
        )


banner("Scenario 2. Tighter support")

printlns(
    "Raising support keeps only patterns backed by more joined tuples. "
    "Higher thresholds produce fewer but more reliable conditions."
)

support_levels = (1, 2, 3, 5, 8)
support_runs = [mine_cure(error=0.5, support=s) for s in support_levels]
for sup, (_, cs) in zip(support_levels, support_runs):
    total = sum(c.conditions_number() for c in cs)
    print(f"  support={sup}:  {len(cs)} CIND(s), {total} condition(s)")
print()

printlns(
    "In practice the workflow is iterative: start with a small support to "
    "see what patterns exist, then raise it to focus on the patterns that "
    "are well-supported by the data."
)


banner("Scenario 3. Cure vs CINDERELLA")

printlns(
    "On the same AINDs the two miners use different scoring and produce "
    "different sets of conditions. CINDERELLA filters multi-attribute "
    "patterns by validity and completeness; Cure mines pairwise patterns "
    "with a support threshold and merges them into a minimal cover."
)

algo_cind, cinderella_cinds = mine_cinderella(error=0.5, validity=0.75, completeness=0.25)
total_cinderella = sum(c.conditions_number() for c in cinderella_cinds)

algo_cure, cure_cinds = mine_cure(error=0.5, support=2)
total_cure = sum(c.conditions_number() for c in cure_cinds)

print(f"  CINDERELLA (validity>=0.75, completeness>=0.25): "
      f"{len(cinderella_cinds)} CIND(s), {total_cinderella} condition(s)")
print(f"  Cure       (support>=2):                          "
      f"{len(cure_cinds)} CIND(s), {total_cure} condition(s)")
print()

printlns(
    "Below is a side-by-side look at one CIND - the conditional patterns "
    "the two miners produce for the same underlying IND."
)


def find_by_ind(cinds, ind_string):
    for c in cinds:
        if c.get_ind_string() == ind_string:
            return c
    return None


def pick_demo_ind(cinderella_cinds, cure_cinds):
    """Pick an IND that has conditions from both miners,
    preferring one where Cure produces a disjunction."""
    cure_inds = {c.get_ind_string(): c for c in cure_cinds if c.conditions_number()}
    cind_inds = {c.get_ind_string(): c for c in cinderella_cinds if c.conditions_number()}
    common = [k for k in cure_inds if k in cind_inds]
    for k in common:
        if any("," in v for cond in cure_inds[k].get_conditions() for v in cond.data()):
            return k
    return common[0] if common else None


def show_conditions(cind, prefer_disjunction=False, max_show=3):
    if cind is None:
        print("    (no CIND with conditions)")
        return
    print(f"    IND: {fmt_ind(cind.get_ind_string())}")
    attrs = fmt_attrs(cind.get_condition_attributes())
    print_attrs_line("conditional attributes", attrs, indent=4)
    conditions = list(cind.get_conditions())
    if prefer_disjunction:
        conditions.sort(key=lambda c: not any("," in v for v in c.data()))
    for j, cond in enumerate(conditions[:max_show], start=1):
        print_condition(cond, j, indent=6)
    if len(conditions) > max_show:
        print(f"      ... ({len(conditions) - max_show} more)")


demo_ind = pick_demo_ind(cinderella_cinds, cure_cinds)
print("  CINDERELLA (validity>=0.75, completeness>=0.25):")
show_conditions(find_by_ind(cinderella_cinds, demo_ind))
print()
print("  Cure (support>=2):")
show_conditions(find_by_ind(cure_cinds, demo_ind), prefer_disjunction=True)
print()

printlns(
    "Two structural differences are visible. First, CINDERELLA's conditional "
    "attributes are taken only from the LHS table, while Cure also includes "
    "the RHS table's columns - patterns can constrain values on both sides "
    "of the inclusion. Second, Cure can collapse several RHS values for the "
    "same LHS key into one comma-separated slot (e.g. \"a, b\"), while "
    "CINDERELLA emits one explicit pattern per distinct value combination."
)


banner("See also")

print("Related primitives in Desbordante:")
print("  * CIND mining (CINDERELLA, PLI-CIND) -  examples/basic/mining_cind.py")
print("  * CIND verification                  -  examples/basic/verifying_cind.py")
print("  * IND mining                         -  examples/basic/mining_ind.py")
print("  * AIND mining                        -  examples/basic/mining_aind.py")
print("  * IND/AIND verification              -  examples/basic/verifying_ind_aind.py")
print()

print("References:")
print("  [1] L. Bravo, W. Fan, S. Ma. Extending Dependencies with Conditions.")
print("      VLDB 2007, pp. 243-254.  -- introduces CINDs.")
print("  [2] J. Bauckmann, Z. Abedjan, U. Leser, H. Muller, F. Naumann.")
print("      Discovering Conditional Inclusion Dependencies. CIKM 2012,")
print("      pp. 2094-2098.  -- CINDERELLA and PLI-CIND.")
print("  [3] O. Cure. Improving the Data Quality of Drug Databases using")
print("      Conditional Dependencies and Ontologies. ACM JDIQ 4(1):20, 2012.")
print("      -- the Cure algorithm used in this example.")
print()
