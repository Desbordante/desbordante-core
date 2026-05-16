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

The example shows how to find exact functional dependencies, approximate
dependencies and relaxed dependencies using different similarity metrics.
We also show how to define custom metrics and how to use RFDs for error
detection.
"""

from collections import defaultdict 
import desbordante
import pandas as pd
import itertools
from tabulate import tabulate
import textwrap
import os

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


def prints(s, width=80):
    print(textwrap.fill(s, width=width))


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
                print(f"{RED}{line}{RESET}")
            else:
                print(line)
        else:
            print(line)
    print()


def fmt_rfd(rfd, col_names):
    lhs_cols = [col_names[i] for i in range(len(col_names)) if rfd.lhs_mask & (1 << i)]
    rhs_col = col_names[rfd.rhs_index]
    lhs_str = ", ".join(lhs_cols) if lhs_cols else "()"
    return {
        "LHS": lhs_str,
        "RHS": rhs_col,
        "conf": f"{rfd.confidence:.3f}",
        "supp": f"{rfd.support:.3f}"
    }

def print_rfds_table(rfds, col_names, title=None):
    if title:
        print(f"{YELLOW}{title}{RESET}")
    if not rfds:
        print("   (none)\n")
        return

    raw_lines = []
    for idx, rfd in enumerate(sorted(rfds, key=lambda r: (r.rhs_index, r.lhs_mask)), start=1):
        lhs_cols = [col_names[i] for i in range(len(col_names)) if rfd.lhs_mask & (1 << i)]
        lhs_str = ", ".join(lhs_cols) if lhs_cols else "()"
        rhs_col = col_names[rfd.rhs_index]
        line = f"[{lhs_str}] -> [{rhs_col}]  (conf={rfd.confidence:.3f}, supp={rfd.support:.3f})"
        numbered_line = f"{idx:>2}. {line}"
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

def print_link(text, url):
    osc8_start = f"\033]8;;{url}\a"
    osc8_end = "\033]8;;\a"
    print(f"{osc8_start}{text}{osc8_end}", end='')


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
    f"SEBD 2017{RESET}."
)
print("  ", end='')
print_link("Read", "https://ceur-ws.org/Vol-2037/paper_22.pdf")
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
    f"  {GREEN}Hypothesis for this example:{RESET} we assume that if people have " +
    "similar height and weight, then they will also have similar shoe sizes. " +
    "We will test this hypothesis using the GA-RFD algorithm."
)
printlns(
    "  In terms of RFD, we expect to discover a dependency of the form " +
    f"{BOLD}[height_cm, weight_kg] -> shoe_size_eu{RESET} with high confidence. "
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
Desbordante provides three ready-to-use metrics:
  {BOLD}equality_metric(){RESET} - returns 1 if the two values are exactly equal, 
                                   0 otherwise;
  {BOLD}abs_diff_metric(){RESET} - for numeric attributes: 1 - |x-y| / max(|x|,|y|).
  {BOLD}levenshtein_metric(){RESET} - for strings: 1 - edit_distance(x,y) / max(len(x), len(y)).

You can also supply any Python function f(a,b)->float as a custom metric.
""")

eq = desbordante.rfd.equality_metric()
abs_diff = desbordante.rfd.abs_diff_metric()
lev = desbordante.rfd.levenshtein_metric()

# ------------------------------------------------------------
# 6. Exact FDs
# ------------------------------------------------------------
banner("Exact FDs (minconf=1.0, equality metrics)", num=6)

print_table(df, title="Sample data - note duplicate weights in rows 1-2 and 5-6", 
            highlight_rows=[0, 1, 4, 5])
algo_fd = desbordante.rfd.algorithms.GaRfd()
algo_fd.load_data(table=(DATA_PATH, ",", True))
algo_fd.set_option("max_generations", 100)
algo_fd.set_option("seed", 42)
algo_fd.execute()
fds = algo_fd.get_rfds()

print_rfds_table(fds, COL_NAMES, title=f"Found {len(fds)} exact FD(s) with minconf=1.0")

printlns(
    "  With equality metrics and minconf = 1.0, GA-RFD recovers classical FDs. " + 
    "Note that support is not 1.0 — it is the fraction of all tuple pairs that " + 
    "have identical values on the left side. On this small dataset, only a few " + 
    "pairs match exactly on any single column, so support is low."
)

printlns(f"{YELLOW}>>> Why does [weight_kg] -> [height_cm] have conf=1.000 and supp=0.071?{RESET}")
printlns(
    "  There are 8 rows, therefore 8*7/2 = 28 tuple pairs. " + 
    "Only two pairs share the same weight: (row 1, row 2) with weight 70, " + 
    "and (row 5, row 6) with weight 81. In both pairs the height is also equal " + 
    "(175 and 178 respectively). Hence, among the 2 pairs that agree on the left side, " + 
    "all 2 agree on the right side → confidence = 2/2 = 1.0. " + 
    "Support = 2/28 ≈ 0.071 because the whole dependency holds for exactly 2 pairs."
)

# ------------------------------------------------------------
# 7. Approximate FDs (lowering confidence)
# ------------------------------------------------------------
banner("Approximate FDs (AFDs): lowering confidence minconf", num=7)

print_table(df)
algo_afd = desbordante.rfd.algorithms.GaRfd()
algo_afd.load_data(table=(DATA_PATH, ",", True))
algo_afd.set_option("minconf", 0.6)
algo_afd.set_option("max_generations", 100)
algo_afd.set_option("seed", 42)
algo_afd.execute()
afds = algo_afd.get_rfds()

print_rfds_table(afds, COL_NAMES, title=f"Found {len(afds)} AFD(s) with minconf>=0.6")

printlns(
    "  When we use equality metrics but lower minconf below 1.0, " + 
    "the RFD pattern reduces to an Approximate Functional Dependency (AFD). " + 
    "A minconf of 0.6 means we accept dependencies that hold in at least 60% " + 
    "of the cases. The resulting dependencies are exactly the classical "
    "approximate FDs."
)

printlns(f"{YELLOW}>>> Why does [height_cm] -> [shoe_size_eu] have conf=0.750 and supp=0.107?{RESET}")
printlns(
    "  There are 4 pairs with identical height: (1,2), (1,3), (2,3) from height 175 " + 
    "and (5,6) from height 178. Among them, the first three also share the same shoe size (40), " + 
    "but the pair (5,6) has different shoe sizes (42 vs 41). Hence confidence = 3/4 = 0.75. " + 
    "Support = 3/28 ≈ 0.107 because three pairs satisfy both sides."
)

# ------------------------------------------------------------
# 8. Relaxed FDs with abs_diff metric
# ------------------------------------------------------------
banner("Relaxed FDs (RFDs): using abs_diff and min_similarity=[0.8]", num=8)

printlns(
    f"{YELLOW}Explanation{RESET}: Now we allow 'similar' rather than 'equal' values by " + 
    "setting min_similarity = 0.8 (values within about 20% " + 
    "are considered similar) and using abs_diff_metric() for all " + 
    "three columns."
)
prints("On this example:")
printlns(
    "  A tuple pair (t1,t2) satisfies the antecedent if, for each "
    "X-attribute, abs_diff(t1[A], t2[A]) >= 0.8."
)

print_table(df)
algo_rfd = desbordante.rfd.algorithms.GaRfd()
algo_rfd.load_data(table=(DATA_PATH, ",", True))
algo_rfd.set_metrics([abs_diff, abs_diff, abs_diff])
algo_rfd.set_option("min_similarity", [0.8])
algo_rfd.set_option("minconf", 0.7)
algo_rfd.set_option("max_generations", 100)
algo_rfd.set_option("seed", 42)
algo_rfd.execute()
rfds = algo_rfd.get_rfds()

print_rfds_table(rfds, COL_NAMES,
                 title=f"Found {len(rfds)} RFD(s) with min_similarity=0.8, minconf>=0.7")

print("For example, the algorithm finds")
print(f"\n  {BOLD}[height_cm, weight_kg] -> shoe_size_eu{RESET}, it means:\n")
printlns(
    "'if two people have similar height " + 
    "and weight (within ~20%), then their shoe sizes are also " + 
    "similar'. This is useful for data imputation, anomaly detection, "
    "or schema understanding."
)

printlns(f"{YELLOW}>>> Why does [height_cm] -> [shoe_size_eu] have conf=1.000 and supp=1.000?{RESET}")
printlns(
    "  With min_similarity = 0.8 almost every pair of rows is considered similar in height " + 
    "(the smallest relative difference is larger than 0.8 for all pairs). Hence support " + 
    "is 1.0. All those pairs also have a relative shoe size difference >= 0.8, " + 
    "so confidence = 1.0. The rule is trivially satisfied because the threshold " + 
    "is too permissive for this dataset."
)

# ------------------------------------------------------------
# 9. Custom metric: Jaccard on 2-grams (string data with typos)
# ------------------------------------------------------------
banner("Custom metric: Jaccard on 2-grams (with typos)", num=9)

printlns(
    "You can pass any Python function f(a,b)->float as a metric. " +
    "Here we use the Jaccard coefficient on sets of character 2-grams."
)
prints("  Jaccard(s1,s2) = |grams(s1) INTERSECT grams(s2)| / |grams(s1) UNION grams(s2)|")
printlns(
    "This is robust to small typos: for example, 'Le Petit Cafe' and 'La Petite Cafe' " +
    "share many 2-gram, so their Jaccard similarity is > 0."
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


algo_custom = desbordante.rfd.algorithms.GaRfd()
algo_custom.load_data(table=(JACCARD_DATA_PATH, ",", True))
algo_custom.set_metrics([jaccard_2gram, eq, eq])
algo_custom.set_option("min_similarity", [0.6])
algo_custom.set_option("minconf", 0.0001)
algo_custom.set_option("max_generations", 150)
algo_custom.set_option("population_size", 2000)
algo_custom.set_option("seed", 42)
algo_custom.execute()
custom_rfds = algo_custom.get_rfds()

print_rfds_table(custom_rfds, COL_NAMES_STR,
                 title=f"RFDs found with Jaccard metric on data with typo")

printlns(
    "  Notice that even with the typos, the algorithm still finds some RFDs, " + 
    "but their confidence is lower than with exact equality. " + 
    "Jaccard on 2-grams treats 'Le Petit Cafe' and 'LePetitCafe' as partially " + 
    "similar, but the similarity is not 1.0, so " + 
    "the resulting dependencies are weaker. You can adjust min_similarity to " + 
    "control the tolerance to typos."
)

# ------------------------------------------------------------
# 10. Error detection in a single dirty dataset
# ------------------------------------------------------------
banner("Error detection and data cleaning (single dirty dataset)", num=10)

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
print_rfds_table(clean_rfds, COL_NAMES_STR, title=f"Found {len(rfds)} RFD(s) on clean data")

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
print_rfds_table(discovered_rfds, COL_NAMES_STR, title=f"Found {len(rfds)} RFD(s) on dirty data")
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

printlns("  Errors detected:")
for report in reports:
    rfd = report["rfd"]
    lhs_names = [COL_NAMES_STR[i] for i in range(len(COL_NAMES_STR)) if rfd.lhs_mask & (1 << i)]
    rhs_name = COL_NAMES_STR[rfd.rhs_index]
    rule_str = f"[{', '.join(lhs_names)}] -> [{rhs_name}] (conf={rfd.confidence:.3f}, supp={rfd.support:.3f})"
    printlns(f"* Rule: {rule_str}")

    top_violations = report["violations"][:3]  # show only top 3
    for j, (idx, count) in enumerate(top_violations):
        prints(f"  Tuple #{idx+1} | Violating pairs: {count} | ")
        prints(f"  Data: {dirty_df.iloc[idx].to_dict()}")
        
        if j < len(top_violations) - 1:
            print("-" * 80)
    print()

printlns(f"{YELLOW}>>> Why do the RFD sets differ, and how do we spot errors?{RESET}")
printlns(
    "  On clean data, [cuisine] -> [district] holds with confidence 1.0 "
    "because every cuisine appears in only one district. "
    "After adding rows 10-11 (Italian in Uptown), the confidence drops to 0.6. "
    "This change signals a potential inconsistency."
)
printlns(
    "  For each RFD discovered on the dirty dataset we count how many times "
    "a tuple violates the rule (LHS similar, RHS dissimilar). Tuples with "
    "the highest violation counts are the best candidates for manual review. "
    "In our example, rows 10 and 11 are flagged precisely because they "
    "introduced Italian cuisine into Uptown, breaking the previously clean "
    "pattern."
)

# ------------------------------------------------------------
# 11. Reproducibility note
# ------------------------------------------------------------
banner("A note on reproducibility", num=11)
printlns(
    "  GA-RFD uses randomness for initialization and evolution. To get the " + 
    "same results across runs, always set the seed parameter: " + 
    "algo.set_option('seed', 42). Without a fixed seed, two runs with the " + 
    "same parameters may return slightly different sets of RFDs."
)

# ------------------------------------------------------------
# 12. Summary
# ------------------------------------------------------------
banner("Summary", num=12)

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

banner("See also")

print("Related primitives in Desbordante:")
print("  * FD mining                -  examples/basic/mining_fd.py")
print("  * AFD mining               -  examples/basic/mining_afd.py")
print("  * MFD verifying            -  examples/basic/verifying_mfd.py") 
print("  * MD mining                -  examples/basic/mining_md.py")
print("  * Mining FD/AFD via GA-RFD -  examples/advanced/...")
print()

print(f"\n{GREEN}Next: try GA-RFD on your own dataset!{RESET}\n")
