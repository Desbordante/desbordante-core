import desbordante
import pandas as pd


GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"


def print_clusters(verifier, data, lhs, rhs):
    print(f"Number of clusters violating FD: {verifier.get_num_error_clusters()}")
    for i, highlight in enumerate(verifier.get_highlights(), start=1):
        print(f"{BLUE_CODE} #{i} cluster: {DEFAULT_COLOR_CODE}")
        for el in highlight.cluster:
            print(f"{el}: {data[data.columns[lhs]][el]} -> {data[data.columns[rhs]][el]}")
        
        print(f"Most frequent rhs value proportion: {highlight.most_frequent_rhs_value_proportion}")
        print(f"Num distinct rhs values: {highlight.num_distinct_rhs_values}\n")


def print_results_for_fd(verifier, data, lhs, rhs):
    if verifier.fd_holds():
        print(GREEN_CODE, "FD holds", DEFAULT_COLOR_CODE)
    else:
        print(RED_CODE, "FD does not hold", DEFAULT_COLOR_CODE)
        print_clusters(verifier, data, lhs, rhs)
        # print(f"But the same {GREEN_CODE} AFD with error threshold = {verifier.get_error()} holds{DEFAULT_COLOR_CODE}")


def print_results_for_afd(verifier, error):
    if verifier.get_error() < error:
        print(GREEN_CODE, "AFD with this error threshold holds", DEFAULT_COLOR_CODE)
    else:
        print(RED_CODE, "AFD with this error threshold does not hold", DEFAULT_COLOR_CODE)
        print(f"But the same {GREEN_CODE} AFD with error threshold = {verifier.get_error()} holds{DEFAULT_COLOR_CODE}")


def exact_scenario(table='examples/datasets/duplicates_short.csv'):
    print("First, let's look at the duplicates_short.csv table and try to verify the functional dependency in it.\n")

    data = pd.read_csv(table, header=[0])
    print(data)

    algo = desbordante.afd_verification.algorithms.Default()
    algo.load_data(table=data)
    
    print(DEFAULT_COLOR_CODE)
    # Verifying exact FD (holds)
    print("Checking whether [id] -> [name] FD holds")

    algo.execute(lhs_indices=[0], rhs_indices=[2])
    print_results_for_fd(algo, data, 0, 2)

    # Verifying exact FD (does not hold)
    print("Checking whether [name] -> [credit_score] FD holds")

    algo.execute(lhs_indices=[1], rhs_indices=[2])
    print_results_for_fd(algo, data, 1, 2)
    print("We learned that in this case the specified FD does not hold and there are two "
          "clusters of rows that contain values that prevent our FD from holding. "
          f"A {BLUE_CODE}cluster{DEFAULT_COLOR_CODE} (with respect to a fixed FD) is a collection "
          "of rows that share the same left-hand side part but differ on the right-hand side one.")
    print("Let's take a closer look at them.\n")
    print('In the first cluster, three values are "0" and a single one is "nan". '
          'This suggests that this single entry with the "nan" value is a result of a mistake by someone '
          'who is not familiar with the table population policy. Therefore, it should probably be changed to "0".\n')
    print("Now let's take a look at the second cluster. "
          'There are two entries: "27" and "28". In this case, it is probably a typo, since buttons 7 and 8 are located '
          "close to each other on the keyboard.\n")
    print("Having analyzed these clusters, we can conclude that our FD does not hold due to typos in the data. "
          "Therefore, by eliminating them, we can get this FD to hold (and make our dataset error-free).")


def approximate_scenario(table='examples/datasets/DnD.csv'):
    print("-" * 80)
    print("Now let's look at the DnD.csv to consider the AFD\n")

    data = pd.read_csv(table, header=[0])
    print(data, end="\n\n")
    
    algo = desbordante.afd_verification.algorithms.Default()
    algo.load_data(table=data)
    algo.execute(lhs_indices=[0], rhs_indices=[1])
    
    # Verifying approximate FD (error threshold sufficient)
    print("Checking whether [Creature] -> [Strength] AFD holds (error threshold = 0.5)")
    print_results_for_afd(algo, 0.5)

    # Verifying approximate FD (error threshold insufficient)
    print("Checking whether [Creature] -> [Strength] AFD holds (error threshold = 0.1)")
    print_results_for_afd(algo, 0.1)

    print("\nSimilarly to the FD verification primitive, the AFD one can provide a user with clusters:\n")
    print_clusters(algo, data, 0, 1)


exact_scenario()
print()
approximate_scenario()
