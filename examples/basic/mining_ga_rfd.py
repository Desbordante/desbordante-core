"""
Example: Mining Relaxed Functional Dependencies (RFDs) with GaRfd.

Uses a genetic algorithm to discover approximate functional dependencies that
allow for fuzzy comparison of values using domain‑specific similarity metrics.

The iris.csv file is assumed to have NO header row (standard UCI format).
If your file *does* have a header, remove `header=None` and `names=...`.
"""

import desbordante
import pandas

# ------------------------------------------------------------
# Prepare the data
# ------------------------------------------------------------
TABLE_PATH = 'examples/datasets/iris.csv'

print("=" * 70)
print("Mining Relaxed Functional Dependencies (RFDs) with GaRfd")
print("=" * 70)

# Load the data WITHOUT a header, and assign meaningful column names
COLUMN_NAMES = ['sepal_length', 'sepal_width', 'petal_length', 'petal_width', 'species']
df = pandas.read_csv(TABLE_PATH, header=None, names=COLUMN_NAMES)

print("\nTable used in this example (Iris dataset):")
print(df.head(10))
print(f"\nShape: {df.shape[0]} rows × {df.shape[1]} columns")
print("Columns:", list(df.columns))

# ------------------------------------------------------------
# GaRfd basics
# ------------------------------------------------------------
print("\n" + "─" * 70)
print("GaRfd is a genetic algorithm that searches for RFDs of the form:")
print("    X (with similarity constraints) → Y (with similarity constraints)")
print("where X and Y are sets of columns and the constraints define")
print("how similar values must be in order for the dependency to hold.")
print("It optimizes both the left‑hand side (LHS) and the similarity thresholds.")
print("─" * 70)

# ------------------------------------------------------------
# First run: built‑in metrics
# ------------------------------------------------------------
print("\n>>> FIRST RUN: using built‑in metrics")

# Instantiate the algorithm
algo = desbordante.rfd.algorithms.GaRfd()
# The third parameter 'False' means "no header" – consistent with pandas reading
algo.load_data(table=(TABLE_PATH, ',', False))

# Define similarity metrics for each column (the order matches COLUMN_NAMES!)
# Column 0: sepal length      → absolute difference (numeric)
# Column 1: sepal width        → absolute difference
# Column 2: petal length       → absolute difference
# Column 3: petal width        → absolute difference
# Column 4: species            → exact equality (string)
abs_diff = desbordante.rfd.abs_diff_metric()
eq      = desbordante.rfd.equality_metric()
metrics = [abs_diff, abs_diff, abs_diff, abs_diff, eq]
algo.set_metrics(metrics)

# Set algorithm parameters
algo.set_option('rfd_min_similarity', 0.8)   # minimum allowed similarity between 0 and 1
algo.set_option('minconf', 0.9)              # minimum confidence for a dependency
algo.set_option('population_size', 22)       # genetic algorithm population size
algo.set_option('rfd_max_generations', 10)   # maximum generations to evolve
algo.set_option('seed', 42)                  # random seed for reproducibility

# Execute the search
algo.execute()

# Retrieve and display found RFDs
rfds = algo.get_rfds()
print(f"\nFound {len(rfds)} RFD(s):")
for i, rfd in enumerate(rfds, 1):
    print(f"  {i}. {rfd}")

# Explain what the output means
print("\nInterpretation:")
print("  For example, if an RFD says:")
print("    (petal_length ≥ 0.9) & (petal_width ≥ 0.9) → species = 1.0")
print("  it means: if two rows have petal_length with similarity >= 0.9 AND")
print("  petal_width with similarity >= 0.9, then the species values must")
print("  be exactly equal (similarity = 1.0) with confidence 0.9.")
print("  'Similarity' is defined by the chosen metric (here abs_diff gives")
print("  similarity = 1 - normalized difference).")

# ------------------------------------------------------------
# Second run: custom Python metric
# ------------------------------------------------------------
print("\n" + "─" * 70)
print(">>> SECOND RUN: injecting a custom Python metric")

# Define a custom similarity function (Jaccard for strings)
def jaccard_sim(a: str, b: str) -> float:
    """Jaccard coefficient on the sets of characters."""
    set_a = set(a)
    set_b = set(b)
    if not set_a and not set_b:
        return 1.0
    return len(set_a & set_b) / len(set_a | set_b)

# Create a new algorithm instance
algo2 = desbordante.rfd.algorithms.GaRfd()
algo2.load_data(table=(TABLE_PATH, ',', False))

# Use set_metrics_py to include the Python function.
# Keep abs_diff for the numeric columns and use our Jaccard function for the species column.
algo2.set_metrics_py([abs_diff, abs_diff, abs_diff, abs_diff, jaccard_sim])

# Use slightly different parameters for variety
algo2.set_option('rfd_min_similarity', 0.8)
algo2.set_option('minconf', 0.9)
algo2.set_option('population_size', 15)
algo2.set_option('rfd_max_generations', 10)
algo2.set_option('seed', 123)
algo2.execute()

rfds2 = algo2.get_rfds()
print(f"С пользовательской метрикой найдено {len(rfds2)} зависимостей")
for i, rfd in enumerate(rfds2):
    print(f"{i+1}. {rfd}")

import desbordante
import pandas
import logging

# logging.basicConfig(level=logging.DEBUG, format='%(asctime)s [%(name)s] %(message)s')

# Чтобы запустить
# PYTHONPATH=build/src/python_bindings python3 examples/basic/mining_ga_rfd.py

# ===================================================
# Пример использования алгоритма GaRfd для поиска
# релаксированных функциональных зависимостей (RFD)
# ===================================================

TABLE = 'examples/datasets/iris.csv'

# Загружаем таблицу для просмотра
df = pandas.read_csv(TABLE)
print("Таблица Iris:")
print(df.head(), "\n")

# ---------- Первый пример (встроенные метрики) ----------
algo = desbordante.rfd.algorithms.GaRfd()

lev = desbordante.rfd.levenshtein_metric()
eq  = desbordante.rfd.equality_metric()
abs_diff = desbordante.rfd.abs_diff_metric()
algo.set_metrics([abs_diff, abs_diff, abs_diff, lev, eq])

algo.load_data(table=(TABLE, ',', False))

algo.set_option('rfd_min_similarity', 0.8)
algo.set_option('minconf', 0.9)
algo.set_option('population_size', 22)
algo.set_option('rfd_max_generations', 10)
algo.set_option('seed', 42)
algo.execute()

rfds = algo.get_rfds()
print(f"Найдено {len(rfds)} релаксированных функциональных зависимостей:")
for i, rfd in enumerate(rfds):
    print(f"{i+1}. {rfd}")

# ---------- Второй пример (пользовательская метрика) ----------
print("\n--- Пример с пользовательской метрикой (Jaccard) ---")
def jaccard_sim(a: str, b: str) -> float:
    set_a = set(a)
    set_b = set(b)
    if not set_a and not set_b:
        return 1.0
    return len(set_a & set_b) / len(set_a | set_b)

algo2 = desbordante.rfd.algorithms.GaRfd()
algo2.load_data(table=(TABLE, ',', False))

algo2.set_metrics_py([jaccard_sim, lev, lev, jaccard_sim, eq])   # ← set_metrics_py!
algo2.set_option('rfd_min_similarity', 0.8)
algo2.set_option('minconf', 0.9)
algo2.set_option('population_size', 10)
algo2.set_option('rfd_max_generations', 10)
algo2.set_option('seed', 42)
algo2.execute()

rfds2 = algo2.get_rfds()
print(f"С пользовательской метрикой найдено {len(rfds2)} зависимостей")
for i, rfd in enumerate(rfds2):
    print(f"{i+1}. {rfd}")
