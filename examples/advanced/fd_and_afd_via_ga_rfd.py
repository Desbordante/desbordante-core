"""
Example: Mining FDs / AFDs via GA-RFD and verifying AFDs - Advanced
====================================================================

This advanced example extends the basic (`mining_ga_rfd.py`) tutorial.
It uses GA-RFD to mine exact functional dependencies (FDs) and 
approximate FDs (AFDs), and then validates the discovered AFDs with 
Desbordante's dedicated AFD verifier. We compare the confidence reported
by GA-RFD with the g1 error computed by the verifier, showing how these 
two measures relate.

The algorithm is based on the paper:
  L. Caruccio, V. Deufemia, G. Polese.
  "A genetic algorithm to discover relaxed functional dependencies from data".
  SEBD 2017, Symposium on Advanced Database Systems.
"""

import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

# ------------------------------------------------------------
# Styling utilities
# ------------------------------------------------------------
YELLOW = "\033[1;33m"
CYAN = "\033[1;36m"
GREEN = "\033[1;32m"
RED = "\033[1;31m"
BLUE = "\033[1;34m"
BOLD = "\033[1m"
ITALIC = "\033[3m"
RESET = "\033[0m"


def prints(s, width=80, end='\n'):
    print(textwrap.fill(s, width=width), end=end)


def printlns(s, width=80):
    prints(s, width)
    print()


def banner(title, num=None):
    prefix = f"{num}. " if num is not None else ""
    print("\n" + "=" * 80)
    print(f"{CYAN}{prefix}{title}{RESET}")
    print("=" * 80)


def print_table(df, title=None, show_index=True, highlight_rows=None):
    if title:
        print(f"\n{YELLOW}{title}{RESET}")
    if show_index:
        display_df = df.reset_index(drop=True)
        display_df.index += 1
        display_df.index.name = "#"
    else:
        display_df = df

    table_str = tabulate(display_df, headers="keys", tablefmt="psql", showindex=show_index)

    lines = table_str.split('\n')
    for i, line in enumerate(lines):
        if highlight_rows and i > 2:
            display_row_num = i - 2
            if (display_row_num - 1) in highlight_rows:
                print(f"{BLUE}{line}{RESET}")
            else:
                print(line)
        else:
            print(line)
    print()


def make_rfd_key(lhs_list, rhs):
    return (tuple(lhs_list), rhs)


def print_rfds_table(rfds, col_names, title=None, highlight=None, color=YELLOW):
    if title:
        print(f"{YELLOW}{title}{RESET}")
    if not rfds:
        print("   (none)\n")
        return

    if highlight is None:
        highlight = set()

    raw_lines = []
    for idx, rfd in enumerate(sorted(rfds, key=lambda r: (r.rhs, r.lhs)), start=1):
        lhs_str = ", ".join(rfd.lhs) if rfd.lhs else "()"
        line = f"[{lhs_str}] -> [{rfd.rhs}]  (conf={rfd.confidence:.3f}, supp={rfd.support:.3f})"
        numbered_line = f"{idx:>2}. {line}"
        if (tuple(rfd.lhs), rfd.rhs) in highlight:
            raw_lines.append(f"{color}" + numbered_line + f"{RESET}")
        else:
            raw_lines.append(numbered_line)

    max_len = 0
    for line in raw_lines:
        pos = line.find('(conf=')
        if pos != -1:
            max_len = max(max_len, pos)
        else:
            max_len = max(max_len, len(line))

    for line in raw_lines:
        pos = line.find('(conf=')
        if pos != -1:
            lhs_part = line[:pos]
            rhs_part = line[pos:]
            padded = lhs_part.ljust(max_len + 2) + rhs_part
            print(padded)
        else:
            print(line)
    print()


# ------------------------------------------------------------
# 1. Introduction
# ------------------------------------------------------------
banner("Introduction", num=1)

printlns(
    "  This example is intended for users who want to dive deeper into the " +
    "relaxed functional dependencies (RFDs). We strongly recommend going " +
    "through the basic RFD example first (examples/basic/mining_ga_rfd.py) " +
    "to become familiar with the core concepts and usage in code. In this " +
    "example we explore the relationship between RFDs and exact/approximate FDs. " +
    "To discover RFDs we employ GA-RFD algorithm and to validate AFDs we use the " +
    "verifier that computes the g1 error."
)
printlns(
    "  By the end you will understand the difference between the confidence " +
    "reported by GA-RFD (based on tuple pairs) and the g1 error from the " +
    "AFD verifier (based on tuple pairs), and how these two measures correspond."
)

# ------------------------------------------------------------
# 2. Dataset (the same as in basic)
# ------------------------------------------------------------
banner("Dataset", num=2)

DATA_PATH = "examples/datasets/rfd/sample_height_weight.csv"
COL_NAMES = ["height_cm", "weight_kg", "shoe_size_eu"]

df = pd.read_csv(DATA_PATH, header=0)
print_table(df, title="Sample data (8 persons, 3 numeric attributes)")

printlns(
    f"  {GREEN}Dataset description:{RESET} This dataset contains information about 8 people. " +
    "Each row represents one person with three numeric attributes:"
)
prints(f"  * {BOLD}height_cm{RESET} — person's height in centimeters")
prints(f"  * {BOLD}weight_kg{RESET} — person's weight in kilograms")
prints(f"  * {BOLD}shoe_size_eu{RESET} — European shoe size")
print()

# ------------------------------------------------------------
# 3. Exact FDs (minconf=1.0, equality metrics)
# ------------------------------------------------------------
banner("Exact FD discovery (minconf=1.0, equality metrics)", num=3)

printlns(
    "The GA-RFD algorithm can also mine classic exact functional dependencies. " +
    "To do this, set minconf = 1.0 and use the default equality metric. "
)

printlns("First, let us look once more at the sample data, this time noting the duplicates in rows 1-2 and 5-6.")
print_table(df, highlight_rows=[0, 1, 4, 5])
algo_fd = desbordante.rfd.algorithms.GaRfd()
algo_fd.load_data(table=(DATA_PATH, ",", True))
algo_fd.execute(max_generations=100, seed=42)
fds = algo_fd.get_rfds()

highlight_fd = make_rfd_key(["weight_kg"], "height_cm")

printlns("Now lets try to mine dependencies with these settings.")
print_rfds_table(fds, COL_NAMES, title=f"Found {len(fds)} exact FD(s) with minconf=1.0",
                 highlight={highlight_fd})

printlns(f"{YELLOW}Why does [weight_kg] -> [height_cm] have conf=1.000 and supp=0.071?{RESET}")
printlns(
    "  There are 8 rows, therefore 8*7/2 = 28 tuple pairs. " + 
    "Only two pairs share the same weight: (row 1, row 2) with weight 70, " + 
    "and (row 5, row 6) with weight 81. In both pairs the height is also equal " + 
    "(175 and 178 respectively). Hence, among the 2 pairs that agree on the left side, " + 
    "both agree on the right side => confidence = 2/2 = 1.0. " + 
    "Support = 2/28 ≈ 0.071 because the whole dependency holds for exactly 2 pairs."
)

# ------------------------------------------------------------
# 4. Approximate FD discovery: lowering minconf
# ------------------------------------------------------------
banner("Approximate FD discovery: lowering minconf", num=4)

printlns(
    "  If we keep equality metrics but lower minconf below 1.0, the Relaxed Functional Dependency " +
    "becomes an Approximate Functional Dependency with similarity metric g1. " +
    "Setting minconf = 0.6 means we accept AFDs that hold in at least 60% of the cases. " + 
    "Thus, the GA-RFD algorithm can search for AFD pattern too. "
)

algo_afd = desbordante.rfd.algorithms.GaRfd()
algo_afd.load_data(table=(DATA_PATH, ",", True))
algo_afd.execute(minconf=0.6, max_generations=100, seed=42)
afds = algo_afd.get_rfds()

highlight_afd = make_rfd_key(["height_cm"], "shoe_size_eu")
printlns("Let us try to check their presence in the same dataset.")
print_rfds_table(afds, COL_NAMES, title=f"Found {len(afds)} AFD(s) with minconf>=0.6",
                 highlight={highlight_afd})

printlns(f"{YELLOW}Why does [height_cm] -> [shoe_size_eu] have conf=0.750 and supp=0.143?{RESET}")
printlns(
    "  There are 4 pairs with identical height: (1,2), (1,3), (2,3) from height 175 " +
    "and (5,6) from height 178. Among them, the first three also share the same shoe size (40), " +
    "but the pair (5,6) has different shoe sizes (42 vs 41). Hence confidence = 3/4 = 0.75. " +
    "Support (LHS) = 4/28 ≈ 0.143 because four pairs have identical height, which matches the "
    "value shown in the table above."
)

# ------------------------------------------------------------
# 5. Verifying AFDs with the AFD verifier (g1 error)
# ------------------------------------------------------------
banner("Verifying AFDs with the AFD verifier (g1 error)", num=5)

printlns(
    "  An AFD can be quantified by its g1 error: the fraction of all tuple " +
    "pairs (i, j) that violate the dependency — that is, pairs where the " +
    "left-hand side attributes are equal but the right-hand side differ. " +
    "Desbordante provides a dedicated AFD verifier that computes exactly " +
    "this measure. We will verify each AFD discovered by GA-RFD and compare " +
    "the g1 error with the confidence value."
)

verifier = desbordante.afd_verification.algorithms.Default()
verifier.load_data(table=(DATA_PATH, ",", True))

table_data = []
for rfd in sorted(afds, key=lambda r: (r.rhs_index, r.lhs_indices)):
    lhs_indices = list(rfd.lhs_indices)
    rhs_index = rfd.rhs_index
    if not lhs_indices:
        continue

    verifier.execute(lhs_indices=lhs_indices, rhs_indices=[rhs_index])
    g1_error = verifier.get_error()
    confidence = rfd.confidence
    support = rfd.support

    lhs_names = [COL_NAMES[i] for i in lhs_indices]
    rhs_name = COL_NAMES[rhs_index]
    rule_str = f"[{', '.join(lhs_names)}] -> [{rhs_name}]"

    table_data.append([
        rule_str,
        f"{confidence:.3f}",
        f"{support:.3f}",
        f"{g1_error:.3f}",
        f"{1 - confidence:.3f}"
    ])

print(f"\n{YELLOW}Verification results:{RESET}\n")
headers = ["rule", "conf", "supp", "g1 error", "1 - conf"]
print(tabulate(table_data, headers=headers, tablefmt="psql",
               colalign=("center", "left", "left", "left", "left")))
print()

printlns(f"{YELLOW}Observations{RESET}")
printlns(
    "  The table compares the confidence reported by GA-RFD with the g1 error " +
    "from the verifier. Confidence is defined as the fraction of pairs with " +
    "equal LHS that also have equal RHS. The g1 error, on the other hand, is " +
    "the fraction of all possible pairs in the dataset that violate the rule " +
    "(LHS equal, RHS different)."
)
printlns(
    "  Because they are computed over different sets of pairs, 1 - Confidence and g1 error " +
    "generally do not match. For example, in our 8-row dataset there are " +
    "8*7/2 = 28 total pairs. For the dependency [height_cm] => [shoe_size_eu] " +
    "only 4 pairs agree on height. Among those, 3 also agree on shoe size, " +
    "so confidence = 3/4 = 0.75, and 1 - confidence = 0.25. However, the " +
    "number of violating pairs is just 1 (rows 5 and 6), which gives a g1 " +
    "error of 1/28 ≈ 0.036 — exactly the value shown by the verifier."
)
printlns(
    "  This illustrates the important difference: g1 error gives a global, " +
    "pair-based measure of how much the data deviates from a perfect FD, " +
    "while confidence tells us how reliable the dependency is among the " +
    "tuples that actually share the LHS values."
)

# ------------------------------------------------------------
banner("Summary")

prints("  In this advanced example we:")
prints(
    "  * Mined exact FDs and approximate FDs using GA-RFD with equality metrics."
)
prints(
    "  * Verified the AFDs with the AFD verifier, computing the g1 error and " +
    "comparing it to the confidence reported by the mining algorithm."
)
printlns(
    "  * Understood the difference: confidence is based on tuple pairs, g1 error " +
    "is the fraction of violating tuple pairs. Both are useful, but the verifier gives " + 
    "a direct measure of data quality at the pair level."
)
prints(
    "  When using RFDs for data cleaning, you can first mine approximate dependencies " +
    "with GA-RFD, then pass them to the verifier to obtain exact pair-level error " +
    "statistics."
)

# ------------------------------------------------------------
banner("See also")

print("Related patterns in Desbordante:")
print("  * FD mining     -  examples/basic/mining_fd.py")
print("  * AFD mining    -  examples/basic/mining_afd.py")
print("  * MFD verifying -  examples/basic/verifying_mfd.py") 
print("  * MD mining     -  examples/basic/mining_md.py")
print("  * RFD mining    -  examples/basic/mining_ga_rfd.py")
print()
