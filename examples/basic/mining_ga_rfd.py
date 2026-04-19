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
