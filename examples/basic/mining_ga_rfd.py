"""
Example: Discovering Relaxed Functional Dependencies with GA-RFD
================================================================

In this example we demonstrate the GA-RFD algorithm from Desbordante,
a genetic algorithm for mining Relaxed Functional Dependencies (RFDs)
from tabular data.  

The algorithm is based on the paper:
  L. Caruccio, V. Deufemia, G. Polese.
  "A genetic algorithm to discover relaxed functional dependencies from data".
  SEBD 2017, Symposium on Advanced Database Systems.

The example shows how to find relaxed dependencies using different similarity metrics.
We also show how to define custom metrics and how to use RFDs for error
detection.
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

def print_table(df, title=None, show_index=True, highlight_rows=None, keep_index=False):
    if title:
        print(f"\n{YELLOW}{title}{RESET}")
    if show_index:
        if keep_index:
            display_df = df.copy()
            display_df.index = display_df.index + 1
        else:
            display_df = df.reset_index(drop=True)
            display_df.index += 1
        display_df.index.name = "#"
    else:
        display_df = df

    table_str = tabulate(display_df, headers="keys", tablefmt="psql", showindex=show_index)

    lines = table_str.split('\n')

    for line in lines:
        if highlight_rows and line.startswith('|'):
            idx_field = line.split('|')[1].strip()
            if idx_field.isdigit() and int(idx_field) in highlight_rows:
                print(f"{BLUE}{line}{RESET}")
                continue
        print(line)
    print()

def make_rfd_key(col_names, lhs_list, rhs):
    mask = 0
    for col in lhs_list:
        mask |= 1 << col_names.index(col)
    rhs_idx = col_names.index(rhs)
    return (mask, rhs_idx)

def print_rfds_table(rfds, col_names, title=None, highlight=None, color=YELLOW):
    if title:
        print(f"{YELLOW}{title}{RESET}")
    if not rfds:
        print("   (none)\n")
        return

    if highlight is None:
        highlight = set()

    raw_lines = []
    for idx, rfd in enumerate(sorted(rfds, key=lambda r: (r.rhs_index, r.lhs_mask)), start=1):
        lhs_cols = [col_names[i] for i in range(len(col_names)) if rfd.lhs_mask & (1 << i)]
        lhs_str = ", ".join(lhs_cols) if lhs_cols else "()"
        rhs_col = col_names[rfd.rhs_index]
        line = f"[{lhs_str}] -> [{rhs_col}]  (conf={rfd.confidence:.3f}, supp={rfd.support:.3f})"
        numbered_line = f"{idx:>2}. {line}"
        if (rfd.lhs_mask, rfd.rhs_index) in highlight:
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

def print_link(text, url, end=''):
    osc8_start = f"\033]8;;{url}\a"
    osc8_end = "\033]8;;\a"
    print(f"{osc8_start}{text}{osc8_end}", end=end)


# ------------------------------------------------------------
# 1. Introduction
# ------------------------------------------------------------
banner("Introduction", num=1)

printlns(
    "  In this example we will learn the basics of RFD mining from tables. " +
    "RFD (Relaxed Functional Dependency) is a pattern that captures " +
    f"the rule: {ITALIC}'if two tuples are similar on a set of attributes X, " +
    f"then they are likely similar on attribute Y'{RESET}. Similarity is defined " +
    "via configurable metrics and thresholds, making RFD more flexible " +
    "than classical functional dependencies."
)
printlns(
    "  This pattern is similar to FD (exact Functional Dependencies), " +
    "AFD (Approximate FDs), but differs in that " +
    "it allows per-attribute similarity metrics and controls acceptable " +
    "deviations via the min_similarity and minconf parameters."
)
prints(
    f"  This pattern is formally defined in the paper: " +
    f"{BOLD}L. Caruccio, V. Deufemia, G. Polese. " +
    "'A genetic algorithm to discover relaxed functional dependencies from data'. " +
    f"SEBD 2017{RESET}.", end=' '
)
print('(', end='')
print_link("Read", "https://ceur-ws.org/Vol-2037/paper_22.pdf")
print(')', end='')
print("\n")

printlns(
    f"{YELLOW}!?{RESET}  It is important not to confuse RFD with the broader term 'approximate FD'. " +
    "Here RFD refers to a concrete pattern defined by Caruccio et al. that " + 
    "combines a similarity metric for each column with a global coverage threshold."
)

# ------------------------------------------------------------
# 2. What is an RFD?
# ------------------------------------------------------------
banner("What is an RFD?", num=2)

print(f"{YELLOW}2.1. Pattern definition{RESET}")
printlns(
    "A Relaxed Functional Dependency (RFD) is a specific pattern of the form"
)
printlns("  X (similarity constraints) => Y (similarity constraints)")
prints(
    "where X and Y are sets of columns. To conclude the definition we need two things:"
)
prints("  1) for each column we indicate a similarity metric")
printlns("  2) we set a single global threshold which indicates the fraction of tuple pairs, conforming to the antecedent X, for which the rule holds (confidence).")
prints("Informally, the dependency means:")
printlns(
    f"  {ITALIC}'If two tuples are similar on X, then they are likely similar on Y.'{RESET}"
)
prints(
    "* Using equality metrics and setting confidence = 1.0 gives us exact FDs."
)
printlns("* Lowering confidence gives us AFDs.")

print(f"{YELLOW}2.2. Confidence and support{RESET}")
printlns("Two numbers describe an RFD.")
printlns(
    f"  {BOLD}Support{RESET}, denoted as supp(X), represents the ratio of tuple pairs that are " +
    "similar on all attributes in X. It characterizes the rule's coverage: the " +
    "larger it is, the more records from the table the rule captures."
)
printlns(
    f"  {BOLD}Confidence{RESET} of a rule X->Y is the fraction of pairs that are similar on X and Y," +
    " divided by the number of pairs that are similar on X. In other words, " +
    f"{BOLD}conf (X->Y) = supp (X UNION Y) / supp (X){RESET}. Confidence tells us how reliable the rule is. It " +
    "returns 1 if and only if, whenever tuple pairs are similar on attributes in X, " +
    "they are also similar on Y. "
)

# ------------------------------------------------------------
# 3. Dataset
# ------------------------------------------------------------
banner("Dataset", num=3)

DATA_PATH = "examples/datasets/sample_original_from_paper.csv"
COL_NAMES = ["height_cm", "weight_kg", "shoe_size_eu"]

df = pd.read_csv(DATA_PATH, header=0)
print_table(df, title="Sample data (7 persons, 3 numeric attributes)")

printlns(
    f"  {GREEN}Dataset description:{RESET} This dataset contains information about 7 people. " +
    "Each row represents one person with three numeric attributes:"
)
prints(f"  * {BOLD}height_cm{RESET}    — person's height in centimeters")
prints(f"  * {BOLD}weight_kg{RESET}    — person's weight in kilograms")
prints(f"  * {BOLD}shoe_size_eu{RESET} — European shoe size")
print()

printlns(
    f"  {GREEN}Hypothesis:{RESET} we assume that " +
    f"{ITALIC}if people have similar height and weight, then they will also " +
    f"have similar shoe sizes{RESET}. " +
    "We will test this hypothesis using the GA-RFD algorithm."
)
printlns(
    "  In terms of RFD, we expect to discover a dependency of the form " +
    f"{GREEN}[height_cm, weight_kg] -> [shoe_size_eu]{RESET} with high confidence. "
)

# ------------------------------------------------------------
# 4. GA-RFD algorithm and key parameters
# ------------------------------------------------------------
banner("GA-RFD algorithm and key parameters", num=4)

printlns(
    "  GA-RFD (Genetic Algorithm for Relaxed Functional Dependencies) evolves " + 
    "a population of candidate RFDs. Each individual encodes a left-hand side " + 
    "(a set of attributes) and a right-hand side (a single attribute). The " + 
    "fitness of an individual is the confidence of the candidate."
)
prints("The main parameters you can set:")
print("""
  population_size       - number of individuals (default 1024)
  max_generations       - number of iterations (default 32)
  crossover_probability - chance of combining two parents (in [0,1], 
                          default 1.0)
  mutation_probability  - chance of random change (in [0,1], default 1.0)
  minconf               - minimum confidence (in [0,1], default 1.0)
  min_similarity        - similarity threshold(s) for relaxed comparisons.
                          Accepts a single value (applied to all columns)
                          or a list of values (one per column). Values 
                          must be in [0,1]. (default {1.0, 1.0, ...})
  seed                  - seed for reproducible results (default 123)
  cache_size            - maximum number of cached comparisons, the bigger 
                          the faster the algorithm will be (default 10000)
""")
printlns(
    f"{GREEN}Note:{RESET} the paper that introduced GA-RFD evaluates the probabilities 0.85/0.3 " + 
    "for crossover/mutation; Desbordante defaults to 1.0/1.0. Tune them for your data."
)
printlns(
    "  The algorithm returns every candidate whose confidence is at least minconf, " + 
    "without pruning subsumed rules: a minimal cover is not computed. " + 
    "If you need a minimal cover, apply post-processing on the discovered RFDs."
)
printlns(
    f"  Because GA-RFD uses randomness, always set the {BOLD}seed{RESET} if you need " + 
    "reproducible results."
)

print(f"{YELLOW}Setting up similarity metrics{RESET}")
printlns(
    f"  Finally, to invoke the algorithm, you must specify which similarity metrics " +
    "should be used by the target RFDs. For this, we employ the set_metrics() method, " +
    "which takes a list of metric functions — one per column in the mined table. For example:"
)
printlns(
    "  " + f"{BOLD}algo.set_metrics([abs_diff, abs_diff, equality]){RESET}"
)
printlns(
    "  This assigns absolute difference metric to the first two columns " +
    "and equality metric to the third column."
)

# ------------------------------------------------------------
# 5. Built-in similarity metrics
# ------------------------------------------------------------
banner("Built-in similarity metrics", num=5)

print(f"""
Desbordante provides four ready-to-use metrics:

  {BOLD}abs_diff_metric(){RESET}          - for numeric attributes: 
                               1 - |x-y| / max(|x|,|y|), clamped to >= 0;
  {BOLD}abs_threshold_metric(diff){RESET} - for numeric attributes: 
                               1 if |x-y| <= diff, else 0;
  {BOLD}equality_metric(){RESET}          - returns 1 if the two values are exactly equal, 
                               else 0;
  {BOLD}levenshtein_metric(){RESET}       - for strings: 
                               1 - edit_distance(x,y) / max(len(x), len(y)).

You can also supply any Python function f(a,b)->float as a custom metric.
""")
abs_diff = desbordante.rfd.abs_diff_metric()
abs_thresh = desbordante.rfd.abs_threshold_metric  # factory function
eq = desbordante.rfd.equality_metric()
lev = desbordante.rfd.levenshtein_metric()

# ------------------------------------------------------------
# 6. Relaxed FDs with abs_diff metric
# ------------------------------------------------------------
banner("Trying RFD discovery: using abs_diff and min_similarity=[0.95]", num=6)

printlns(
    f"{YELLOW}Explanation:{RESET} Let's try to mine true[{YELLOW}*{RESET}] RFDs in the above-mentioned " +
    "table. This means that we are interested in RFDs whose values within each column are " +
    "'similar' rather than 'equal'. Suppose that we consider values within about " +
    f"{BOLD}5%{RESET} to be similar. Therefore, we need to set " +
    f"{BOLD}min_similarity = 0.95{RESET} and use abs_diff_metric() " +
    "for all three columns."
)
printlns(
    f"{YELLOW}*{RESET} The relationship between FD and RFD is discussed in a separate example given " +
    "in the references."
)

printlns(
    "  Thus, a tuple pair (t1,t2) satisfies the antecedent of a dependency R (X->Y) " +
    "if, for each attribute A IN X, abs_diff(t1[A], t2[A]) >= 0.95. If for each tuple " +
    "pair satisfying the antecedent, abs_diff(t1[Y], t2[Y]) >= 0.95, then we say that this " +
    "rule satisfies the succedent of R. If, in addition to the above, the minconf of R is greater " +
    "than a user-specified threshold, then we say that R holds on this table."
)

print_table(df)
algo_rfd = desbordante.rfd.algorithms.GaRfd()
algo_rfd.load_data(table=(DATA_PATH, ",", True))
algo_rfd.set_metrics([abs_diff, abs_diff, abs_diff])
algo_rfd.execute(min_similarity=[0.95], minconf=0.7, max_generations=500, seed=42)
rfds = algo_rfd.get_rfds()

highlight_key = make_rfd_key(COL_NAMES, ["height_cm", "weight_kg"], "shoe_size_eu")
printlns("Let's run the algorithm to discover suitable RFDs.")
print_rfds_table(rfds, COL_NAMES,
                 title=f"Found {len(rfds)} RFDs with min_similarity=[0.95], minconf>=0.7")

printlns(
    "  The search finds 4 RFDs: the height column dominates because its pairs are " +
    "the most similar under the relative 5% threshold. The natural rule " +
    f"{YELLOW}[height_cm, weight_kg] -> [shoe_size_eu]{RESET} is NOT among them: " +
    "jointly similar (height, weight) pairs are rare, and only 3 of the 6 such pairs " +
    f"also have similar shoe sizes, so its confidence is {RED}0.5{RESET}, " +
    f"below {BOLD}minconf=0.7{RESET}. " +
    "In the next section we will recover this rule using stricter absolute thresholds."
)

# ------------------------------------------------------------
# 7. Verifying hypothesis
# ------------------------------------------------------------
banner("Verifying hypothesis", num=7)

printlns(
    f"  {GREEN}Recall our hypothesis from Section 3:{RESET} we expect that similar height and " +
    "weight imply similar shoe size. The absolute metric lets us define " +
    "'similar' in concrete, measurable terms."
)
printlns(
    f"  {YELLOW}To check it we set the following attribute difference thresholds:{RESET} " +
    f"{BOLD}height <= 1 cm, weight <= 10 kg, shoe size <= 1{RESET}. " +
    f"This models {BOLD}'people of practically the same height and roughly the same " +
    f"weight should have almost the same shoe size'{RESET}. " +
    "Since the abs_threshold_metric returns 0 or 1, we set min_similarity=1.0 to accept only exact " +
    "matches according to these thresholds."
)

print_table(df)
algo_abs = desbordante.rfd.algorithms.GaRfd()
algo_abs.load_data(table=(DATA_PATH, ",", True))
algo_abs.set_metrics([abs_thresh(1.0), abs_thresh(10.0), abs_thresh(1.0)])
algo_abs.execute(min_similarity=[1.0], minconf=0.5, max_generations=500, seed=42)
abs_rfds = algo_abs.get_rfds()

highlight_key = make_rfd_key(COL_NAMES, ["height_cm", "weight_kg"], "shoe_size_eu")
printlns("Let's rerun the algorithm with these parameters.")
print_rfds_table(abs_rfds, COL_NAMES,
                 title="RFDs with absolute thresholds (minconf=0.5)",
                 highlight={highlight_key}, color=GREEN)

printlns(
    f"  The key dependency {GREEN}[height_cm, weight_kg] -> [shoe_size_eu]{RESET} " +
    f"has {BOLD}confidence=1.000{RESET} and {BOLD}support=0.286{RESET}. " +
    "It tells us: among pairs that differ by at most 1 cm in height and 10 kg in weight," +
    f" the shoe size {BOLD}always{RESET} differs by no more than 1. We say 'always' " +
    "since the confidence of this dependency is 1. " +
    "This is a clear, actionable rule for data quality or prediction."
)
printlns(
    f"  {GREEN}This confirms our hypothesis:{RESET} people with very similar height and " +
    "weight (according to our chosen absolute thresholds) indeed have " +
    "almost the same shoe size. The discovered RFD gives a precise, " +
    "quantitative formulation of that intuitive relationship."
)
prints(
    "  Because the thresholds are strict, only a few pairs have similar LHS parts; " +
    "hence the support of this rule is low, but the confidence can still be high. " +
    "The absolute metric makes the similarity definition completely transparent."
)

# ------------------------------------------------------------
# 8. Custom metric: Jaccard on 2-grams (string data with typos)
# ------------------------------------------------------------
banner("Custom metric: Jaccard on 2-grams (with typos)", num=8)

printlns(
    "You can pass any Python function f(a,b)->float as a metric. " +
    "To demonstrate it, we use the Jaccard coefficient on sets of character 2-grams."
)
printlns("  Jaccard(s1,s2) = |grams(s1) INTERSECT grams(s2)| / |grams(s1) UNION grams(s2)|")
prints(
    f"This is robust to small typos: for example, {ITALIC}‘Le Petit Cafe'{RESET} and " +
    f"{ITALIC}‘La Petite Cafe'{RESET} " +
    "share many 2-grams, so their Jaccard similarity is > 0."
)

JACCARD_DATA_PATH = "examples/datasets/jaccard_err_data.csv"
COL_NAMES_STR = ["restaurant", "cuisine", "district"]
jaccard_df = pd.read_csv(JACCARD_DATA_PATH)
print_table(jaccard_df, title="String dataset with a typo:")


def jaccard_2gram(a, b) -> float:
    def ngrams(s, n=2):
        s = str(s).lower()
        return {s[i:i+n] for i in range(max(1, len(s)-n+1))}
    set_a, set_b = ngrams(a), ngrams(b)
    if not set_a and not set_b:
        return 1.0
    return len(set_a & set_b) / len(set_a | set_b)


algo_eq = desbordante.rfd.algorithms.GaRfd()
algo_eq.load_data(table=(JACCARD_DATA_PATH, ",", True))
algo_eq.set_metrics([eq, eq, eq])
algo_eq.execute(min_similarity=[1.0], minconf=0.0001, max_generations=150,
                population_size=2000, seed=42)
eq_rfds = algo_eq.get_rfds()

printlns(
    "Let's first try to discover RFDs using exact string equality on all " +
    "columns (no Jaccard used)."
)
print_rfds_table(eq_rfds, COL_NAMES_STR,
                 title="RFDs with exact equality on all columns")
printlns(
    "  Without fuzzy matching, the only dependencies found involve cuisine " +
    "and district because they contain exact duplicates. On the other hand, restaurant names, " +
    "which are all unique due to typos, never appear in any found RFD."
)

algo_jac = desbordante.rfd.algorithms.GaRfd()
algo_jac.load_data(table=(JACCARD_DATA_PATH, ",", True))
algo_jac.set_metrics([jaccard_2gram, eq, eq])
algo_jac.execute(min_similarity=[0.3], minconf=0.0001, max_generations=150,
                 population_size=2000, seed=42)
jac_rfds = algo_jac.get_rfds()

highlight_key = make_rfd_key(COL_NAMES_STR, ["restaurant"], "cuisine")
printlns(
    "Let's try again using fuzzy matching with Jaccard metric."
)
print_rfds_table(jac_rfds, COL_NAMES_STR,
                 title="RFDs with Jaccard on restaurant (min_similarity=0.3)",
                 highlight={highlight_key}, color=GREEN)

printlns(
    "  Now 'restaurant' appears in the dependencies! For instance, " +
    f"{GREEN}[restaurant] -> [cuisine]{RESET} tells us that restaurants with similar names " +
    "tend to serve the same cuisine, even when the names contain small typos. " +
    "This rule was invisible with exact equality. Jaccard on 2-grams " +
    "successfully absorbs spelling variations and keeps the dependency alive."
)
printlns(
    f"  Support is low because very few restaurant-name pairs reach the " +
    f"{BOLD}0.3{RESET} Jaccard threshold, " +
    "but confidence is well above random, indicating a real signal."
)

# ------------------------------------------------------------
# 9. Error detection and data cleaning with GA-RFD
# ------------------------------------------------------------
banner("Error detection and data cleaning with GA-RFD", num=9)

DIRTY_DATA_PATH = "examples/datasets/jaccard_typo_data.csv"
dirty_df = pd.read_csv(DIRTY_DATA_PATH).reset_index(drop=True)

prints(
    f"{YELLOW}Scenario:{RESET} You receive a dataset and want to assess its " +
    "quality. You don't know whether there are errors, or where they are, " +
    f"but you have a domain rule: {ITALIC}each cuisine type should be associated with " +
    f"only one district.{RESET} " +
    "Because the data may contain typos, cuisine values are compared using " +
    "a fuzzy similarity metric. This leads us to working with the following RFD: " +
    f"{BOLD}[cuisine] -> [district]{RESET}."
)

print_table(
    dirty_df,
    title="Dirty dataset:",
    highlight_rows=[10, 11]
)

TARGET_LHS = ["cuisine"]
TARGET_RHS = "district"
RFD_TEXT = f"[{', '.join(TARGET_LHS)}] -> [{TARGET_RHS}]"
TARGET_KEY = make_rfd_key(COL_NAMES_STR, TARGET_LHS, TARGET_RHS)

DISCOVERY_MINCONF = 0.01

# ------------------------------------------------------------
# Similarity thresholds used by GA-RFD.
#
# Column order is:
#   1. restaurant
#   2. cuisine
#   3. district
#
# We relax cuisine similarity to catch typos such as:
#   French vs Freanch
# ------------------------------------------------------------
RESTAURANT_SIMILARITY = 0.3
CUISINE_SIMILARITY = 0.5
DISTRICT_SIMILARITY = 1.0

MIN_SIMILARITY = [
    RESTAURANT_SIMILARITY,
    CUISINE_SIMILARITY,
    DISTRICT_SIMILARITY,
]


def run_garfd(table):
    algo = desbordante.rfd.algorithms.GaRfd()
    if isinstance(table, str):
        algo.load_data(table=(table, ",", True))
    else:
        algo.load_data(table=table)
    algo.set_metrics([jaccard_2gram, eq, eq])
    algo.execute(
        min_similarity=MIN_SIMILARITY,
        minconf=DISCOVERY_MINCONF,
        max_generations=500,
        population_size=2000,
        seed=42,
    )
    return algo.get_rfds()


def get_rfd_by_key(rfds, key):
    for rfd in rfds:
        if (rfd.lhs_mask, rfd.rhs_index) == key:
            return rfd
    return None


# TODO: change to verifier when it becomes available
def find_rfd_violations(df, lhs_cols, rhs_col, lhs_metric, lhs_thresh):
    """
    Finds tuple pairs that are similar on the LHS but differ on the RHS.
    These pairs violate the RFD lhs_cols -> rhs_col.
    """
    violations = []
    lhs_col = lhs_cols[0]

    for i in range(len(df)):
        for j in range(i + 1, len(df)):
            val_i = str(df.iloc[i][lhs_col])
            val_j = str(df.iloc[j][lhs_col])

            if lhs_metric(val_i, val_j) >= lhs_thresh:
                if df.iloc[i][rhs_col] != df.iloc[j][rhs_col]:
                    violations.append((i, j))

    return violations


def clean_and_log(dirty_df, key_col, metric, sim_thresh=0.5):
    """
    Performs the cleaning procedure and returns (clean_df, log_df).

    Rows are grouped by the key column (e.g. restaurant name); for each group
    the most frequent value in every column is taken as the canonical
    (correct) value, and rows that deviate from it are fixed. This encodes the
    usual assumption of data cleaning: typos are rare, so the majority variant
    is right.

    log_df contains one entry per corrected value and uses the same 1-based
    row numbering as the dirty dataset printed in the overview, so the
    corrections can be verified manually.
    """
    clean_rows = []
    log = []

    for _, group in dirty_df.groupby(dirty_df[key_col].str.lower(), sort=False):
        canonical = group.mode(dropna=False).iloc[0]
        clean_rows.append(canonical)

        for idx, row in group.iterrows():
            for col in dirty_df.columns:
                old_val, new_val = row[col], canonical[col]
                if old_val != new_val:
                    correction = (
                        "likely typo"
                        if metric(str(old_val), str(new_val)) >= sim_thresh
                        else "value differs from the canonical row"
                    )
                    log.append({
                        "row": idx + 1,
                        "column": col,
                        "old_value": old_val,
                        "new_value": new_val,
                        "correction": correction,
                    })

    clean_df = pd.DataFrame(clean_rows).reset_index(drop=True)
    return clean_df, pd.DataFrame(log)


# ------------------------------------------------------------
# Step 1: First, let's mine RFDs on the dirty dataset
# ------------------------------------------------------------
printlns(f"{YELLOW}Step 1: First, let's mine RFDs on the dirty dataset{RESET}")

dirty_rfds = run_garfd(DIRTY_DATA_PATH)

print_rfds_table(
    dirty_rfds,
    COL_NAMES_STR,
    title=f"RFDs found on the dirty dataset (minconf={DISCOVERY_MINCONF})",
    highlight={TARGET_KEY},
    color=RED,
)

dirty_target = get_rfd_by_key(dirty_rfds, TARGET_KEY)
dirty_conf = dirty_target.confidence if dirty_target else 0.0

printlns(
    f"  GA-RFD found the rule {BOLD}{RFD_TEXT}{RESET} with confidence " +
    f"{RED}{dirty_conf:.2f}{RESET}."
)

if dirty_conf < 1.0:
    printlns(
        "  Because the confidence is not 1.0, the dataset contains tuple pairs " +
        "that are similar on the LHS but disagree on the RHS. These are " +
        "candidate errors in the RHS."
    )
else:
    printlns("  The expected rule holds perfectly on this dataset.")


# ------------------------------------------------------------
# Step 2: Extract violating rows from the dirty dataset
# ------------------------------------------------------------
printlns(f"{YELLOW}Step 2: Extracting the violating rows{RESET}")

prints(
    f"  An RFD violation occurs when two rows have similar cuisine " +
    f"(Jaccard similarity >= {CUISINE_SIMILARITY}), but different districts."
)

violations = find_rfd_violations(
    dirty_df,
    TARGET_LHS,
    TARGET_RHS,
    jaccard_2gram,
    CUISINE_SIMILARITY,
)

if violations:
    suspicious_rows = sorted({idx for pair in violations for idx in pair})

    print_table(
        dirty_df.iloc[suspicious_rows],
        title="Rows participating in violations (candidate errors):",
        highlight_rows=[10, 11],
        keep_index=True,
    )

    printlns(
        f"{GREEN}Observation:{RESET} The violation check returns all rows participating " +
        "in conflicting pairs. In this example, row 9 is the correct entry, " +
        "while rows 10 and 11 contain dirty values: 'Middown' and 'Freanch'."
    )

else:
    printlns("  No violations found.")


# ------------------------------------------------------------
# Step 3: Cleaning procedure
# ------------------------------------------------------------
printlns(f"{YELLOW}Step 3: The cleaning procedure{RESET}")

printlns(
    "  Now we fix the values that the violations pointed to. " +
    "For each restaurant group we take the most frequent variant as the " +
    "canonical (correct) row and fix the remaining rows to match it, assuming " +
    "that typos are rare, so the majority is right. "
)

clean_dataset, _ = clean_and_log(
    dirty_df,
    key_col="restaurant",
    metric=jaccard_2gram,
)

printlns(
    "  After these two fixes, rows 9-11 become identical " +
    "('Le Petit Cafe', 'French', 'Midtown'), so we keep a single copy and " +
    "drop the duplicates. The result is the cleaned dataset printed in Step 4."
)


# ------------------------------------------------------------
# Step 4: Mine RFDs on the clean dataset
# ------------------------------------------------------------
prints(f"{YELLOW}Step 4: Now let's mine RFDs on the cleaned dataset{RESET}")

print_table(
    clean_dataset,
    title="Cleaned dataset:"
)

clean_rfds = run_garfd(clean_dataset)

print_rfds_table(
    clean_rfds,
    COL_NAMES_STR,
    title=f"RFDs found on the cleaned dataset (minconf={DISCOVERY_MINCONF})",
    highlight={TARGET_KEY},
    color=GREEN,
)

clean_target = get_rfd_by_key(clean_rfds, TARGET_KEY)
clean_conf = clean_target.confidence if clean_target else 0.0

printlns(
    f"  On the cleaned dataset, the rule {BOLD}{RFD_TEXT}{RESET} has confidence " +
    f"{GREEN}{clean_conf:.2f}{RESET}."
)
printlns(
    f"{GREEN}Conclusion:{RESET} The confidence of {RFD_TEXT} changed from " +
    f"{dirty_conf:.2f} on the dirty dataset to {clean_conf:.2f} on the clean dataset. " +
    "This shows how the discovered RFDs differ between dirty and clean data, " +
    "and how GA-RFD can help identify candidate inconsistencies."
)
printlns(
    f"{GREEN}Note:{RESET} If the dataset contains too many errors related to the same dependency, " +
    "error detection may become unreliable. Incorrect values can become the majority, " +
    "so the algorithm treats them as weak rules (because confidence will be low). " +
    "In that case, a more advanced procedure is needed, and it will probably be necessary to " +
    "experiment with the threshold values."
)

# ------------------------------------------------------------
# 10. Reproducibility note
# ------------------------------------------------------------
banner("A note on reproducibility", num=10)
printlns(
    "  GA-RFD uses randomness for initialization and evolution. To get the " + 
    "same results across runs, always set the seed parameter: " + 
    "algo.execute(seed=42). Without a fixed seed, two runs with the " + 
    "same parameters may return slightly different sets of RFDs."
)

# ------------------------------------------------------------
banner("Summary")

printlns(
    "  We have seen how GA-RFD can discover relaxed FDs by adjusting the similarity " +
    "threshold min_similarity and the confidence minconf. " +
    "The choice of similarity metric is crucial: equality gives strict " +
    "comparisons, while absolute difference and Jaccard on character 2-grams " +
    "allow fuzzy matching."
)
printlns(
    "  We also demonstrated how to define a custom metric using Python functions " +
    "and how to use RFDs for error detection. In practice, you would mine RFDs " +
    "on clean data, then score new records: low similarity on the right-hand side " +
    "despite high similarity on the left-hand side flags potential issues."
)
printlns(
    "  When applying GA-RFD to your own data, remember to tune the population size " +
    "and the number of generations for your dataset size. Larger populations and " + 
    "more generations improve recall but increase runtime. Always set a fixed " + 
    "seed for reproducible experiments."
)
printlns(
    "  The next step is to familiarize yourself with the advanced example, where you" +
    "will learn how to configure the algorithm for exact FD and AFD discovery: " +
    f"{BOLD}examples/advanced/fd_and_afd_via_ga_rfd.py{RESET}. Before that, we recommend looking " + 
    "through the other FD/AFD examples."
)


# ------------------------------------------------------------
banner("See also")

print("Related patterns in Desbordante:")
print("  * FD mining                -  examples/basic/mining_fd.py")
print("  * AFD mining               -  examples/basic/mining_afd.py")
print("  * MFD verifying            -  examples/basic/verifying_mfd.py")
print("  * MD mining                -  examples/basic/mining_md.py")
print("  * Mining FD/AFD via GA-RFD -  examples/advanced/fd_and_afd_via_ga_rfd.py")
print()

print(f"\n{GREEN}Next: try GA-RFD on your own dataset!{RESET}\n")