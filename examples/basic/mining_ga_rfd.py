"""
Example: Mining Functional and Relaxed Functional Dependencies with GaRfd.

GaRfd is a genetic algorithm that discovers dependencies between columns.
By adjusting its parameters, the same algorithm can search for:
- Exact functional dependencies (FDs)
- Approximate functional dependencies (AFDs)
- Matching dependencies (MDs) -- dependencies with similarity on the left side
- Relaxed functional dependencies (RFDs) -- dependencies with similarity on both sides
  and relaxed extent (confidence < 1.0)

The example uses the Iris dataset (iris.csv), which is assumed to have
NO header row (standard UCI format). If your file *does* have a header,
remove `header=None` and `names=...`.
"""

import desbordante
import pandas

# ------------------------------------------------------------
# Prepare the data
# ------------------------------------------------------------
TABLE_PATH = 'examples/datasets/iris.csv'

print("=" * 70)
print("Mining dependencies with GaRfd")
print("=" * 70)

COLUMN_NAMES = ['sepal_length', 'sepal_width', 'petal_length', 'petal_width', 'species']
df = pandas.read_csv(TABLE_PATH, header=None, names=COLUMN_NAMES)

print("\nTable used in this example (Iris dataset):")
print(df.head(10))
print(f"\nShape: {df.shape[0]} rows × {df.shape[1]} columns")
print("Columns:", list(df.columns))

# ------------------------------------------------------------
# Built‑in metrics
# ------------------------------------------------------------
lev = desbordante.rfd.levenshtein_metric()    # Levenstain distance
abs_diff = desbordante.rfd.abs_diff_metric()  # 1 – normalized absolute diff
eq = desbordante.rfd.equality_metric()        # exact equality (return 1 or 0)

# ============================================================
# 1. DEFAULT PARAMETERS -> EXACT FUNCTIONAL DEPENDENCIES (FDs)
# ============================================================
print("\n" + "─" * 70)
print("1. EXACT FUNCTIONAL DEPENDENCIES (default behaviour)")
print("   No need to set any options – GaRfd uses equality metrics")
print("   and min_similarity = 1.0, minconf = 1.0 by default.")
print("   This discovers exact FDs.")
print("─" * 70)

algo_fd = desbordante.rfd.algorithms.GaRfd()
algo_fd.load_data(table=(TABLE_PATH, ',', False))
# All options are already set to "exact FD mode"
algo_fd.execute()
fds = algo_fd.get_rfds()
print(f"Found {len(fds)} exact FD(s):")
for i, rfd in enumerate(fds, 1):
    print(f"  {i}.\t{rfd}")

# ============================================================
# 2. APPROXIMATE FUNCTIONAL DEPENDENCIES (AFDs)
#    Lower the confidence threshold (minconf) to allow
#    exceptions
# ============================================================
print("\n" + "─" * 70)
print("2. APPROXIMATE FUNCTIONAL DEPENDENCIES (AFDs)")
print("   Lower minconf to, say, 0.8. The algorithm will now")
print("   accept dependencies that hold in at least 80% of the cases.")
print("   Keep min_similarity = 1.0 for exact value matches.")
print("─" * 70)

algo_afd = desbordante.rfd.algorithms.GaRfd()
algo_afd.load_data(table=(TABLE_PATH, ',', False))
algo_afd.set_option('minconf', 0.8)  # allow 20% violations
algo_afd.execute()
afds = algo_afd.get_rfds()
print(f"Found {len(afds)} AFD(s) with minconf=0.8:")
for i, rfd in enumerate(afds, 1):
    print(f"  {i}.\t{rfd}")

# ============================================================
# 3. MATCHING DEPENDENCIES (MDs) / RELAXED FDs
#    Lower min_similarity to allow fuzzy value comparisons
# ============================================================
print("\n" + "─" * 70)
print("3. RELAXED / MATCHING DEPENDENCIES (RFDs / MDs)")
print("   Lower min_similarity to, say, 0.8 and use a metric")
print("   that measures closeness (e.g. absolute difference).")
print("   This finds dependencies that hold when values are")
print("   'similar enough', not just exactly equal.")
print("─" * 70)

algo_rfd = desbordante.rfd.algorithms.GaRfd()
algo_rfd.load_data(table=(TABLE_PATH, ',', False))
algo_rfd.set_metrics([abs_diff, abs_diff, abs_diff, abs_diff, eq])
algo_rfd.set_option('min_similarity', 0.8)
algo_rfd.set_option('population_size', 128)
algo_rfd.set_option('max_generations', 10)
algo_rfd.execute()
rfds = algo_rfd.get_rfds()
print(f"Found {len(rfds)} RFD(s) with min_similarity=0.8:")
for i, rfd in enumerate(rfds, 1):
    print(f"  {i}.\t{rfd}")

print("\nAdditional possibilities:")
print("  - To search for matching dependencies (MDs) where only the LHS")
print("    comparison is relaxed, you can set the RHS metric to equality")
print("    and min_similarity < 1.0 only for the LHS columns.")
print("  - To discover relaxed FDs with fuzzy RHS, use a similarity metric")
print("    on the RHS as well.")


# ============================================================
# 4. EXPLICIT PARAMETER SETUP FOR RFDs
# ============================================================
print("\n" + "─" * 70)
print("3. EXPLICIT RFD PARAMETER SETUP")
print("   Here we explicitly set every parameter that GaRfd provides.")
print("─" * 70)

algo_explicit = desbordante.rfd.algorithms.GaRfd()
algo_explicit.load_data(table=(TABLE_PATH, ',', False))

# Metrics (default: equality for every column)
algo_explicit.set_metrics([abs_diff, abs_diff, abs_diff, abs_diff, eq])

# Similarity threshold – values are considered similar if similarity >= this
algo_explicit.set_option('min_similarity', 0.8)         # default 1.0

# Minimum confidence for a dependency to be kept (beta)
algo_explicit.set_option('minconf', 0.9)                    # default 1.0s

# Population size – how many individuals per generation
algo_explicit.set_option('population_size', 250)            # default 1024

# Maximum number of generations to evolve
algo_explicit.set_option('max_generations', 10)         # default 32

# Probability of crossing over two parents (0.0 .. 1.0)
algo_explicit.set_option('crossover_probability', 0.49) # default 1.0

# Probability of mutating an individual (0.0 .. 1.0)
algo_explicit.set_option('mutation_probability', 0.85)  # default 1.0

# Random seed – guarantees reproducible results
algo_explicit.set_option('seed', 52)                        # default 42

# Cache size – larger values speed up repeated support queries
algo_explicit.set_option('cache_size', 1000)            # default 10000

algo_explicit.execute()
rfds_explicit = algo_explicit.get_rfds()
print(f"Found {len(rfds_explicit)} RFD(s) with explicit parameters:")
for i, rfd in enumerate(rfds_explicit, 1):
    print(f"  {i}.\t{rfd}")

# ============================================================
# 5. USING A CUSTOM PYTHON METRIC
# ============================================================
print("\n" + "─" * 70)
print("4. CUSTOM METRIC (Python function)")
print("   You can pass any Python callable to set_metrics().")
print("   Here we use the Jaccard coefficient for strings.")
print("─" * 70)

def jaccard_sim(a: str, b: str) -> float:
    set_a = set(a)
    set_b = set(b)
    if not set_a and not set_b:
        return 1.0
    return len(set_a & set_b) / len(set_a | set_b)

algo_custom = desbordante.rfd.algorithms.GaRfd()
algo_custom.load_data(table=(TABLE_PATH, ',', False))
algo_custom.set_metrics([abs_diff, abs_diff, abs_diff, abs_diff, jaccard_sim])
algo_custom.set_option('min_similarity', 0.8)
algo_custom.set_option('minconf', 0.9)
algo_custom.set_option('population_size', 15)
algo_custom.execute()
custom_rfds = algo_custom.get_rfds()
print(f"Found {len(custom_rfds)} RFD(s) with custom metric:")
for i, rfd in enumerate(custom_rfds, 1):
    print(f"  {i}.\t{rfd}")

# ------------------------------------------------------------
# About reproducibility
# ------------------------------------------------------------
print("\n" + "─" * 70)
print("A note on reproducibility")
print("   GaRfd is a non-deterministic algorithm because it uses random")
print("   operations (initialization, crossover, mutation).")
print("   To obtain the same results on every run, always set the 'seed'")
print("   option to a fixed value (e.g., 42).")
print("   Without a fixed seed the algorithm may find different (or even")
print("   fewer) dependencies on different runs.")
print("─" * 70)

# ------------------------------------------------------------
# Final note
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Summary:")
print("  - Default settings -> exact FDs")
print("  - Lower minconf -> AFDs")
print("  - Lower min_similarity + appropriate metrics -> RFDs / MDs")
print("  - Pass Python functions to set_metrics() for custom similarity")
print("  - Adjust cache_size to balance speed/memory")
print("=" * 70)
