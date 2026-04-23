import desbordante
import pandas

# ===================================================
# Пример использования алгоритма GaRfd для поиска
# релаксированных функциональных зависимостей (RFD)
# ===================================================

TABLE = 'examples/datasets/iris.csv'

# Загружаем таблицу для просмотра
df = pandas.read_csv(TABLE)
print("Таблица Iris:")
print(df.head(), "\n")

# 1 Создаём алгоритм GaRfd и загружаем данные
algo = desbordante.rfd.algorithms.GaRfd()
print("17")
algo.load_data(table=(TABLE, ',', True))
print("19")

# 2 Задаём метрики сходства для каждого столбца
#    Можно использовать встроенные метрики или свои функции

# Встроенная метрика Левенштейна
lev = desbordante.rfd.levenshtein_metric()
print("26")
# Встроенная метрика равенства
eq = desbordante.rfd.equality_metric()
print("29")

# Для первых четырёх столбцов (числовые) используем Левенштейна,
# для последнего (variety) – равенство
metrics = [lev, lev, lev, lev, eq]
print("34")
algo.set_metrics(metrics)
print("36")

# 3 Устанавливаем пороги и параметры генетического алгоритма
algo.set_option('rfd_min_similarity', 0.8)   # порог сходства значений
algo.set_option('minconf', 0.9)              # минимальное confidence (β)
algo.set_option('population_size', 30)       # размер популяции
algo.set_option('rfd_max_generations', 10)   # число поколений
algo.set_option('rfd_crossover_probability', 0.85)
algo.set_option('rfd_mutation_probability', 0.3)
algo.set_option('seed', 42)
print("46")

# 4 Запускаем поиск
algo.execute()
print("49")

# 5 Получаем и выводим результаты
rfds = algo.get_rfds()
print(f"Найдено {len(rfds)} релаксированных функциональных зависимостей:")
for i, rfd in enumerate(rfds):
    # rfd – объект с полями lhs_mask, rhs_index, support, confidence
    # Выводим его строковое представление
    print(f"{i+1}. {rfd}")

# Доп пример с пользовательской метрикой
print("\n--- Пример с пользовательской метрикой (Jaccard) ---")
def jaccard_sim(a: str, b: str) -> float:
    set_a = set(a)
    set_b = set(b)
    if not set_a and not set_b:
        return 1.0
    return len(set_a & set_b) / len(set_a | set_b)

# Создаём новый экземпляр алгоритма
algo2 = desbordante.rfd.GaRfd()
algo2.load_data(table=(TABLE, ',', True))
# Первый столбец сравниваем по Jaccard
algo2.set_metrics([jaccard_sim, lev, lev, lev, eq])
algo2.set_option('rfd_min_similarity', 0.8)
algo2.set_option('minconf', 0.9)
algo2.set_option('population_size', 30)
algo2.set_option('rfd_max_generations', 10)
algo2.set_option('seed', 42)
algo2.execute()
rfds2 = algo2.get_rfds()
print(f"С пользовательской метрикой найдено {len(rfds2)} зависимостей")
for i, rfd in enumerate(rfds2):
    print(f"{i+1}. {rfd}")
