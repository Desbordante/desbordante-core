"""
# 1. What primitive is this?
Genetic algorithm for discovering Relaxed Functional Dependencies (GA-RFD)

Based on the paper:
"A genetic algorithm to discover relaxed functional dependencies from data"
by Loredana Caruccio, Vincenzo Deufemia, Giuseppe Polese.
University of Salerno, SEBD 2017, Symposium on Advanced Database Systems.
"""

import desbordante
import pandas as pd
import logging
import os

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(name)s] %(message)s')

# ------------------------------------------------------------
# Definition of a Relaxed Functional Dependency (RFD)
# ------------------------------------------------------------
print("=" * 70)
print("What is a Relaxed Functional Dependency?")
print("=" * 70)
print("""
An RFD has the form:
   X (similarity constraints) => Y (similarity constraints)

X and Y are sets of columns.
For each column we choose a similarity metric and a threshold.
The dependency means:
   "If two tuples are similar on X, then they are similar on Y."

The confidence of an RFD is the fraction of tuple pairs that satisfy this rule.
Lowering the confidence threshold gives Approximate FDs (AFDs).
If we relax the similarity on the left-hand side (X) but require strict
equality on the right-hand side (Y), we obtain a Matching Dependency (MD).
Using equality metrics and confidence = 1.0 gives classical FDs.
""")

# ------------------------------------------------------------
# Algorithm and its parameters
# ------------------------------------------------------------
print("=" * 70)
print("GA-RFD algorithm and parameters")
print("=" * 70)
print("""
GaRfd is a genetic algorithm. It evolves a population of candidate RFDs.
You can set:
  population_size      - number of individuals (default 1024)
  max_generations      - number of iterations (default 32)
  crossover_probability - chance of combining two parents (in [0,1], default 1.0)
  mutation_probability  - chance of random change (in [0,1], default 0.1)
  minconf               - minimum confidence (in [0,1], default 1.0)
  min_similarity        - minimum similarity threshold for relaxed comparisons (in [0,1], default 1.0)
  seed                  - for reproducible results
  cache_size            - maximum number of cached comparisons (default 10000)

The algorithm is non-deterministic due to random initialisation - always set seed
if you need reproducible output.
""")

# ------------------------------------------------------------
# Dataset description
# ------------------------------------------------------------
print("=" * 70)
print("The sample dataset")
print("=" * 70)
print("""
We use a tiny CSV file (examples/datasets/sample_height_weight.csv)
with 7 rows and 3 numeric columns:
  Height (cm), Weight (kg), Shoe size (EU).
""")

DATA_PATH = 'examples/datasets/sample_height_weight.csv'
COL_NAMES = ['height', 'weight', 'shoe_size']
df = pd.read_csv(DATA_PATH, header=None, names=COL_NAMES)
print(df.to_string(index=False))
print(f"\nShape: {df.shape[0]} rows x {df.shape[1]} columns")

# ------------------------------------------------------------
# Built-in similarity metrics
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Built-in similarity metrics")
print("=" * 70)
print("""
GaRfd supports three built-in metrics:
  - equality_metric()        => 1 if values equal, else 0
  - abs_diff_metric()        => 1 - (|x - y| / max(|x|, |y|))
  - levenshtein_metric()     => normalized edit distance for strings

You can also supply your own Python function (see section 9).
""")

eq = desbordante.rfd.equality_metric()
abs_diff = desbordante.rfd.abs_diff_metric()
lev = desbordante.rfd.levenshtein_metric()

# ------------------------------------------------------------
# Exact Functional Dependencies
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Exact FDs (default behaviour)")
print("=" * 70)
algo = desbordante.rfd.algorithms.GaRfd()
algo.load_data(table=(DATA_PATH, ',', True))
algo.set_option('max_generations', 100)
algo.set_option('seed', 42)
algo.execute()
fds = algo.get_rfds()
print(f"Found {len(fds)} exact FD(s):")
for rfd in sorted(fds, key=lambda r: (r.rhs_index, r.lhs_mask)):
    print(f"  {rfd}")

# ------------------------------------------------------------
# Approximate Functional Dependencies (AFDs)
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Approximate FDs (lowering confidence)")
print("=" * 70)
algo_afd = desbordante.rfd.algorithms.GaRfd()
algo_afd.load_data(table=(DATA_PATH, ',', True))
algo_afd.set_option('minconf', 0.1)
algo_afd.set_option('max_generations', 100)
algo_afd.set_option('seed', 42)
algo_afd.execute()
afds = algo_afd.get_rfds()
print(f"Found {len(afds)} AFD(s) with minconf=0.1:")
for rfd in sorted(afds, key=lambda r: (r.rhs_index, r.lhs_mask)):
    print(f"  {rfd}")

# ------------------------------------------------------------
# Relaxed dependencies (using abs_diff)
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Relaxed FDs (RFDs) with abs_diff metric")
print("=" * 70)
print("Now we allow similar (not just equal) values by lowering min_similarity to 0.8.")
algo_rfd = desbordante.rfd.algorithms.GaRfd()
algo_rfd.load_data(table=(DATA_PATH, ',', True))
algo_rfd.set_metrics([abs_diff, abs_diff, abs_diff])
algo_rfd.set_option('min_similarity', 0.8)
algo_rfd.set_option('max_generations', 100)
algo_rfd.set_option('seed', 42)
algo_rfd.execute()
rfds = algo_rfd.get_rfds()
print(f"Found {len(rfds)} RFD(s):")
for rfd in sorted(rfds, key=lambda r: (r.rhs_index, r.lhs_mask)):
    print(f"  {rfd}")
print("""
Interpretation: [Height, Weight] -> Shoe_size means:
  "if two persons have similar height and weight,
   then their shoe size is also similar."
""")

# ------------------------------------------------------------
# Custom metric (Python function)
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Custom similarity metric")
print("=" * 70)
print("We define a Jaccard similarity on string representations of the numbers.")
def jaccard_sim(a, b) -> float:
    set_a = set(str(a))
    set_b = set(str(b))
    if not set_a and not set_b:
        return 1.0
    return len(set_a & set_b) / len(set_a | set_b)

algo_custom = desbordante.rfd.algorithms.GaRfd()
algo_custom.load_data(table=(DATA_PATH, ',', True))
# equality for Height and Weight, Jaccard for Shoe size
algo_custom.set_metrics([eq, eq, jaccard_sim])
algo_custom.set_option('min_similarity', 0.3)
algo_custom.set_option('minconf', 0.4)
algo_custom.set_option('max_generations', 100)
algo_custom.set_option('seed', 42)
algo_custom.execute()
custom_rfds = algo_custom.get_rfds()
print(f"Found {len(custom_rfds)} RFD(s) with custom metric:")
for rfd in sorted(custom_rfds, key=lambda r: (r.rhs_index, r.lhs_mask)):
    print(f"  {rfd}")

# ------------------------------------------------------------
# Error detection and cleaning scenario
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Error detection and data cleaning")
print("=" * 70)
print("""
We deliberately introduce a mistake: change the Shoe size of the first person
from 40 to 47 (a likely typo). This breaks the exact FD [Height,Weight] -> Shoe_size.
After fixing it back, the FD is restored.
""")

typo_df = df.copy()
typo_df.loc[0, 'shoe_size'] = 47
typo_path = 'typo_data.csv'
typo_df.to_csv(typo_path, index=False, header=True)

algo_typo = desbordante.rfd.algorithms.GaRfd()
algo_typo.load_data(table=(typo_path, ',', True))
algo_typo.set_option('max_generations', 100)
algo_typo.set_option('seed', 42)
algo_typo.execute()
typo_fds = algo_typo.get_rfds()
print("Exact FDs on data with typo (fewer than original):")
for rfd in typo_fds:
    print(f"  {rfd}")

# Fix the error back to original
typo_df.loc[0, 'shoe_size'] = 40
fixed_path = 'fixed_data.csv'
typo_df.to_csv(fixed_path, index=False, header=True)

algo_fixed = desbordante.rfd.algorithms.GaRfd()
algo_fixed.load_data(table=(fixed_path, ',', True))
algo_fixed.set_option('max_generations', 100)
algo_fixed.set_option('seed', 42)
algo_fixed.execute()
fixed_fds = algo_fixed.get_rfds()
print("\nAfter fixing the typo (exact FDs should be restored):")
for rfd in fixed_fds:
    print(f"  {rfd}")

os.remove(typo_path)
os.remove(fixed_path)

# ------------------------------------------------------------
# Parameter tuning and reproducibility
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("The importance of seed for reproducibility")
print("=" * 70)
print("""
GaRfd is a genetic algorithm - it uses random numbers to initialise the
population and to perform crossover/mutation. Therefore, two consecutive runs
with exactly the same parameters may yield different sets of RFDs.
"Reproducible results" means that if you fix a seed, the sequence of random
numbers is always the same, and the algorithm produces identical output
on any computer and at any time.

Below we first demonstrate two runs WITHOUT a seed (the results may differ).
Then we run the algorithm twice WITH the same seed - we will see the same output.
""")

# Two runs without seed
print("--- Two runs without seed (results may vary) ---")
for run in [1, 2]:
    algo_noseed = desbordante.rfd.algorithms.GaRfd()
    algo_noseed.load_data(table=(DATA_PATH, ',', True))
    algo_noseed.set_option('minconf', 0.6)
    algo_noseed.set_option('max_generations', 100)
    # no seed set
    algo_noseed.execute()
    res = algo_noseed.get_rfds()
    print(f"Run {run}: {len(res)} RFD(s)")
    for rfd in sorted(res, key=lambda r: (r.rhs_index, r.lhs_mask)):
        print(f"  {rfd}")

# Two runs with seed = 42
print("\n--- Two runs with seed = 42 (results must be identical) ---")
for run in [1, 2]:
    algo_seed = desbordante.rfd.algorithms.GaRfd()
    algo_seed.load_data(table=(DATA_PATH, ',', True))
    algo_seed.set_metrics([eq, eq, eq])
    algo_seed.set_option('minconf', 0.6)
    algo_seed.set_option('max_generations', 100)
    algo_seed.set_option('seed', 42)
    algo_seed.execute()
    res = algo_seed.get_rfds()
    print(f"Run {run}: {len(res)} RFD(s)")
    for rfd in sorted(res, key=lambda r: (r.rhs_index, r.lhs_mask)):
        print(f"  {rfd}")

print("""
As you can see, the random runs may differ in the number and content of RFDs,
while the seeded runs are perfectly reproducible.
""")

# ------------------------------------------------------------
# Summary and next steps
# ------------------------------------------------------------
print("\n" + "=" * 70)
print("Summary")
print("=" * 70)
print("""
- Default settings give exact FDs;
- Lower minconf => AFDs;
- Lower min_similarity + absolute difference metric => RFDs;
- You can pass any Python function as a custom metric;
- Use seed for reproducibility, tune population/generations for better results.
""")
