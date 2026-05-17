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
    "the rule: «if two tuples are similar on a set of attributes X, " +
    "then they are likely similar on attribute Y». Similarity is defined " +
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
    "«A genetic algorithm to discover relaxed functional dependencies from data». " +
    f"SEBD 2017{RESET}.", end=' '
)
print('(', end='')
print_link("Read", "https://ceur-ws.org/Vol-2037/paper_22.pdf")
print(')', end='')
print("\n")

printlns(
    f"{YELLOW}!?{RESET}  It is important not to confuse RFD as a general term for 'approximate FD'. " + 
    "Here RFD refers to a concrete pattern defined by Caruccio et al. that " + 
    "combines similarity metric for each column global coverage threshold."
)

# ------------------------------------------------------------
# 2. What is an RFD?
# ------------------------------------------------------------
banner("What is an RFD?", num=2)

print(f"{YELLOW}>>> 2.1. Pattern definition{RESET}")
printlns(
    "A Relaxed Functional Dependency (RFD) is a specific pattern of the form"
)
printlns("  X (similarity constraints) => Y (similarity constraints)")
printlns(
    "where X and Y are sets of columns. For each column we indicate a " + 
    "similarity metric and we set a global coverage threshold which indicates " + 
    "the fraction of tuple pairs for which the rule holds (confidence)."
)
prints("Informally, the dependency means:")
printlns('  "If two tuples are similar on X, then they are likely similar on Y."')
prints(
    "* Using equality metrics and setting confidence = 1.0 gives us exact FDs."
)
printlns(
    "* Lowering confidence gives us AFDs."
)

print(f"{YELLOW}>>> 2.2. Confidence and support{RESET}")
printlns("Two numbers describe an RFD.")
printlns(
    f"  {BOLD}Confidence{RESET} tells us how reliable the rule is. " +
    "It is the fraction of pairs that are similar on X and Y, " +
    "divided by the number of pairs that are similar on X. " +
    "For example, if confidence = 0.9, then among all pairs that are similar on X, " +
    "90% are also similar on Y."
)
printlns(
    f"  {BOLD}Support{RESET} is the fraction of all tuple pairs that " +
    f"are similar on {BOLD}both X and Y{RESET}. It tells us how often the entire " +
    "dependency holds. If support = 0.5, half of all pairs satisfy the rule."
)

# ------------------------------------------------------------
# 3. Dataset
# ------------------------------------------------------------
banner("Dataset", num=3)

DATA_PATH = "examples/datasets/sample_height_weight.csv"
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

printlns(
    f"  {GREEN}Hypothesis:{RESET} we assume that if people have " +
    "similar height and weight, then they will also have similar shoe sizes. " +
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
  mutation_probability  - chance of random change (in [0,1], default 0.1)
  minconf               - minimum confidence (in [0,1], default 1.0)
  min_similarity        - similarity threshold(s) for relaxed comparisons.
                          Accepts a single value (applied to all columns)
                          or a list of values (one per column). Values 
                          must be in [0,1]. (default {1.0, 1.0, ...})
  seed                  - seed for reproducible results
  cache_size            - maximum number of cached comparisons, the bigger 
                          the faster the algorithm will be (default 10000)
""")
printlns(
    "  Because GA-RFD uses randomness, always set the seed if you need " + 
    "reproducible results."
)

print(f"{YELLOW}>>> Where are similarity metrics defined?{RESET}")
printlns(
    f"  Similarity metrics are set using the {BOLD}set_metrics(){RESET} method, which takes " +
    "a list of metric functions (one per column). For example:"
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

  {BOLD}abs_diff_metric(){RESET} - for numeric attributes: 1 - |x-y| / max(|x|,|y|);
  {BOLD}abs_threshold_metric(diff){RESET} - for numeric attributes: 1 if |x-y| <= diff, else 0;
  {BOLD}equality_metric(){RESET} - returns 1 if the two values are exactly equal, else 0;
  {BOLD}levenshtein_metric(){RESET} - for strings: 1 - edit_distance(x,y) / max(len(x), len(y)).

You can also supply any Python function f(a,b)->float as a custom metric.
""")
abs_diff = desbordante.rfd.abs_diff_metric()
abs_thresh = desbordante.rfd.abs_threshold_metric  # factory function
eq = desbordante.rfd.equality_metric()
lev = desbordante.rfd.levenshtein_metric()

# ------------------------------------------------------------
# 6. Relaxed FDs with abs_diff metric
# ------------------------------------------------------------
banner("Relaxed FDs (RFDs): using abs_diff and min_similarity=[0.95]", num=6)

printlns(
    f"{YELLOW}Explanation{RESET}: Now we allow 'similar' rather than 'equal' values by " + 
    "setting min_similarity = 0.95 (values within about 5% " + 
    "are considered similar) and using abs_diff_metric() for all " + 
    "three columns."
)
prints("On this example:")
printlns(
    "  A tuple pair (t1,t2) satisfies the antecedent if, for each "
    "X-attribute, abs_diff(t1[A], t2[A]) >= 0.95."
)

print_table(df)
algo_rfd = desbordante.rfd.algorithms.GaRfd()
algo_rfd.load_data(table=(DATA_PATH, ",", True))
algo_rfd.set_metrics([abs_diff, abs_diff, abs_diff])
algo_rfd.set_option("min_similarity", [0.95])
algo_rfd.set_option("minconf", 0.7)
algo_rfd.set_option("max_generations", 500)
algo_rfd.set_option("seed", 42)
algo_rfd.execute()
rfds = algo_rfd.get_rfds()

highlight_key = make_rfd_key(COL_NAMES, ["height_cm", "weight_kg"], "shoe_size_eu")
print_rfds_table(rfds, COL_NAMES,
                 title=f"Found {len(rfds)} RFD(s) with min_similarity=[0.95], minconf>=0.7",
                 highlight={highlight_key})

print(f"Let's take {YELLOW}[height_cm, weight_kg] -> [shoe_size_eu]{RESET} and see what it means:")
printlns(
    "  'if two people have similar height " + 
    "and weight (within ~5%), then their shoe sizes are also " + 
    "similar'. This is useful for data imputation, anomaly detection, "
    "or schema understanding."
)

# ------------------------------------------------------------
# 7. Verifying hypothesis
# ------------------------------------------------------------
banner("Verifying hypothesis", num=7)

printlns(
    f"  {YELLOW}Here we set:{RESET} height <= 1 cm, weight <= 10 kg, shoe size <= 1. " +
    f"This models {BOLD}'people of practically the same height and roughly the same " +
    f"weight should have almost the same shoe size'{RESET}. " +
    "Since the metric returns 0 or 1, we set min_similarity=1.0 to accept only exact " +
    "matches according to these thresholds. "
)
printlns(
    f"  {GREEN}Recall our hypothesis from Section 3:{RESET} we expect that similar height and " +
    "weight imply similar shoe size. The absolute metric lets us define " +
    "'similar' in concrete, measurable terms."
)

print_table(df)
algo_abs = desbordante.rfd.algorithms.GaRfd()
algo_abs.load_data(table=(DATA_PATH, ",", True))
algo_abs.set_metrics([abs_thresh(1.0), abs_thresh(10.0), abs_thresh(1.0)])
algo_abs.set_option("min_similarity", [1.0])
algo_abs.set_option("minconf", 0.5)
algo_abs.set_option("max_generations", 500)
algo_abs.set_option("seed", 42)
algo_abs.execute()
abs_rfds = algo_abs.get_rfds()

highlight_key = make_rfd_key(COL_NAMES, ["height_cm", "weight_kg"], "shoe_size_eu")
print_rfds_table(abs_rfds, COL_NAMES,
                 title="RFDs with absolute thresholds (minconf=0.5)",
                 highlight={highlight_key}, color=GREEN)

printlns(
    f"  The key dependency {GREEN}[height_cm, weight_kg] -> [shoe_size_eu]{RESET} " +
    "has confidence=1.000 and support=0.250. " +
    "It tells us: among pairs that differ by at most 1 cm in height, 10 kg in weight, " +
    "the shoe size <= 1. " +
    "This is a clear, actionable rule for data quality or prediction."
)
printlns(
    f"  {GREEN}This confirms our hypothesis:{RESET} people with very similar height and " +
    "weight (according to our chosen absolute thresholds) indeed have " +
    "almost the same shoe size. The discovered RFD gives a precise, " +
    "quantitative formulation of that intuitive relationship."
)

prints(
    "  Because the thresholds are strict, only a few pairs match - hence the " +
    "support is low, but the confidence can still be high. " +
    "The absolute metric makes the similarity definition completely transparent."
)

# ------------------------------------------------------------
# 8. Custom metric: Jaccard on 2-grams (string data with typos)
# ------------------------------------------------------------
banner("Custom metric: Jaccard on 2-grams (with typos)", num=8)

printlns(
    "You can pass any Python function f(a,b)->float as a metric. " +
    "Here we use the Jaccard coefficient on sets of character 2-grams."
)
prints("  Jaccard(s1,s2) = |grams(s1) INTERSECT grams(s2)| / |grams(s1) UNION grams(s2)|")
printlns(
    "This is robust to small typos: for example, 'Le Petit Cafe' and 'La Petite Cafe' " +
    "share many 2-grams, so their Jaccard similarity is > 0."
)

JACCARD_DATA_PATH = "examples/datasets/jaccard_typo_data.csv"
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
algo_eq.set_option("min_similarity", [1.0])
algo_eq.set_option("minconf", 0.0001)
algo_eq.set_option("max_generations", 150)
algo_eq.set_option("population_size", 2000)
algo_eq.set_option("seed", 42)
algo_eq.execute()
eq_rfds = algo_eq.get_rfds()

print_rfds_table(eq_rfds, COL_NAMES_STR,
                 title="RFDs with exact equality on all columns")
printlns(
    "  Without fuzzy matching, the only dependencies found involve cuisine " +
    "and district because they have exact duplicates. Restaurant names, " +
    "which are all unique due to typos, never appear in any rule."
)

algo_jac = desbordante.rfd.algorithms.GaRfd()
algo_jac.load_data(table=(JACCARD_DATA_PATH, ",", True))
algo_jac.set_metrics([jaccard_2gram, eq, eq])
algo_jac.set_option("min_similarity", [0.3])
algo_jac.set_option("minconf", 0.0001)
algo_jac.set_option("max_generations", 150)
algo_jac.set_option("population_size", 2000)
algo_jac.set_option("seed", 42)
algo_jac.execute()
jac_rfds = algo_jac.get_rfds()

highlight_key = make_rfd_key(COL_NAMES_STR, ["restaurant"], "cuisine")
print_rfds_table(jac_rfds, COL_NAMES_STR,
                 title="RFDs with Jaccard on restaurant (min_similarity=0.3)",
                 highlight={highlight_key}, color=GREEN)

printlns(
    "  Now restaurant appears in the dependencies! For instance, " +
    f"{GREEN}[restaurant] -> [cuisine]{RESET} tells us that restaurants with similar names " +
    "tend to serve the same cuisine, even when the names contain small typos. " +
    "This rule was invisible with exact equality. Jaccard on 2-grams " +
    "successfully absorbs spelling variations and keeps the dependency alive."
)
printlns(
    "  Support is low because very few restaurant-name pairs reach the 0.3 Jaccard threshold, " +
    "but confidence is well above random, indicating a real signal."
)

# ------------------------------------------------------------
# 9. Error detection in a single dirty dataset
# ------------------------------------------------------------
banner("Error detection and data cleaning (single dirty dataset)", num=9)

CLEAN_DATA_PATH = "examples/datasets/jaccard_clean_data.csv"
DIRTY_DATA_PATH = "examples/datasets/jaccard_typo_data.csv"

clean_df = pd.read_csv(CLEAN_DATA_PATH)
dirty_df = pd.read_csv(DIRTY_DATA_PATH)

print_table(clean_df, title="Clean dataset:")
algo_clean = desbordante.rfd.algorithms.GaRfd()
algo_clean.load_data(table=(CLEAN_DATA_PATH, ",", True))
algo_clean.set_metrics([jaccard_2gram, eq, eq])
algo_clean.set_option("min_similarity", [0.3, 1.0, 1.0])
algo_clean.set_option("minconf", 0.6)
algo_clean.set_option("max_generations", 1000)
algo_clean.set_option("population_size", 5000)
algo_clean.set_option("seed", 42)
algo_clean.execute()
clean_rfds = algo_clean.get_rfds()

highlight_key = make_rfd_key(COL_NAMES_STR, ["cuisine"], "district")
print_rfds_table(clean_rfds, COL_NAMES_STR,
                 title=f"Found {len(clean_rfds)} RFD(s) on clean data",
                 highlight={highlight_key})

print_table(dirty_df, title="Dirty dataset:")
algo = desbordante.rfd.algorithms.GaRfd()
algo.load_data(table=(DIRTY_DATA_PATH, ",", True))
algo.set_metrics([jaccard_2gram, eq, eq])
algo.set_option("min_similarity", [0.3, 1.0, 1.0])
algo.set_option("minconf", 0.6)
algo.set_option("max_generations", 1000)
algo.set_option("population_size", 5000)
algo.set_option("seed", 42)
algo.execute()
discovered_rfds = algo.get_rfds()

print_rfds_table(discovered_rfds, COL_NAMES_STR,
                 title=f"Found {len(discovered_rfds)} RFD(s) on dirty data",
                 highlight={highlight_key})

printlns(f"{YELLOW}>>> Why do the RFD sets differ, and how to find out if there is an error?{RESET}")
printlns(
    "  On clean data, [cuisine] -> [district] holds with confidence 1.0 " +
    "because every cuisine appears in only one district. " +
    "After adding rows 10-11 (Italian in Uptown), the confidence drops to 0.6. " +
    "This change signals a potential inconsistency."
)

def extract_violations_for_garfd_rfds(df, rfds, metrics, thresholds):
    from collections import defaultdict
    import itertools

    violation_reports = []
    n = len(df)

    for rfd in rfds:
        lhs_indices = [i for i in range(len(df.columns)) if rfd.lhs_mask & (1 << i)]
        rhs_idx = rfd.rhs_index

        if not lhs_indices or rhs_idx >= len(df.columns):
            continue

        violation_count = defaultdict(int)
        for i, j in itertools.combinations(range(n), 2):
            lhs_sim = all(
                metrics[col](str(df.iloc[i, col]), str(df.iloc[j, col])) >= thresholds[col]
                for col in lhs_indices
            )
            if not lhs_sim:
                continue

            rhs_sim = metrics[rhs_idx](str(df.iloc[i, rhs_idx]), str(df.iloc[j, rhs_idx])) >= thresholds[rhs_idx]

            if not rhs_sim:
                violation_count[i] += 1
                violation_count[j] += 1

        violation_reports.append({
            "rfd": rfd,
            "violations": sorted(violation_count.items(), key=lambda x: x[1], reverse=True)
        })
    return violation_reports

reports = extract_violations_for_garfd_rfds(
    dirty_df,
    discovered_rfds,
    metrics=[jaccard_2gram, eq, eq],
    thresholds=[0.3, 1.0, 1.0]
)

print("="*80)
printlns("  Errors detected:")
for report in reports:
    rfd = report["rfd"]
    lhs_names = [COL_NAMES_STR[i] for i in range(len(COL_NAMES_STR)) if rfd.lhs_mask & (1 << i)]
    rhs_name = COL_NAMES_STR[rfd.rhs_index]
    rule_str = f"[{', '.join(lhs_names)}] -> [{rhs_name}] (conf={rfd.confidence:.3f}, supp={rfd.support:.3f})"
    if len(report["violations"]):
        prints(f"{RED}Rule: {rule_str}{RESET}")

    top_violations = report["violations"][:3]  # show only top 3
    for j, (idx, count) in enumerate(top_violations):
        prints(f"  Tuple #{idx+1} | Violating pairs: {count} | ")
        prints(f"  Data: {dirty_df.iloc[idx].to_dict()}")
        
        if j < len(top_violations) - 1:
            print("-" * 80)
    print()

printlns(
    f"{GREEN}Note{RESET} that Bella Napoli (row #4) is flagged because it violates the rule " +
    "when paired with the two erroneous Italian rows - it is an innocent bystander " +
    "that helps locate the true errors."
)

printlns(f"{YELLOW}>>> How we spot errors?{RESET}")
printlns(
    "  For each RFD discovered on the dirty dataset we count how many times " +
    "a tuple violates the rule (LHS similar, RHS dissimilar). Tuples with " +
    "the highest violation counts are the best candidates for manual review. " +
    "In our example, rows 10 and 11 are flagged precisely because they " +
    "introduced Italian cuisine into Uptown, breaking the previously clean " +
    "pattern."
)

# ------------------------------------------------------------
# 10. Reproducibility note
# ------------------------------------------------------------
banner("A note on reproducibility", num=10)
printlns(
    "  GA-RFD uses randomness for initialization and evolution. To get the " + 
    "same results across runs, always set the seed parameter: " + 
    "algo.set_option('seed', 42). Without a fixed seed, two runs with the " + 
    "same parameters may return slightly different sets of RFDs."
)

# ------------------------------------------------------------
banner("Summary")

printlns(
    "  We have seen how GA-RFD can discover exact FDs, approximate FDs, and " + 
    "relaxed FDs by adjusting the similarity threshold min_similarity and the confidence " + 
    "minconf. The choice of similarity metric is crucial: equality gives strict " + 
    "comparisons, while absolute difference and Levenshtein allow fuzzy matching."
)
printlns(
    "  We also demonstrated how to define a custom metric using Python functions "
    "and how to use RFDs for error detection. In practice, you would mine RFDs "
    "on clean data, then score new records: low similarity on the right-hand side "
    "despite high similarity on the left-hand side flags potential issues."
)
printlns(
    "  When applying GA-RFD to your own data, remember to tune the population size " +
    "and the number of generations for your dataset size. Larger populations and " + 
    "more generations improve recall but increase runtime. Always set a fixed " + 
    "seed for reproducible experiments."
)

# ------------------------------------------------------------
banner("See also")

print("Related primitives in Desbordante:")
print("  * FD mining                -  examples/basic/mining_fd.py")
print("  * AFD mining               -  examples/basic/mining_afd.py")
print("  * MFD verifying            -  examples/basic/verifying_mfd.py") 
print("  * MD mining                -  examples/basic/mining_md.py")
print("  * Mining FD/AFD via GA-RFD -  examples/advanced/fd_and_afd_via_ga-rfd.py")
print()

print(f"\n{GREEN}Next: try GA-RFD on your own dataset!{RESET}\n")
