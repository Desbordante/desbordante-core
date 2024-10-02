import desbordante
import pandas as pd


GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"
RESET_CODE = "\033[0m"

def print_clusters(verifier, table, indices):
    print(f"Number of clusters violating IND: {len(verifier.get_violating_clusters())}")
    for i, cluster in enumerate(verifier.get_violating_clusters(), start=1):
        print(f"{BLUE_CODE} #{i} cluster: {DEFAULT_COLOR_CODE}")
        for el in cluster:
            values = " ".join([f"{table[table.columns[idx]][el]}" for idx in indices])
            print(f"{el}: {values}")


def print_results_for_ind(verifier):
    if verifier.get_error() == 0:
        print(GREEN_CODE, "IND holds", DEFAULT_COLOR_CODE, RESET_CODE)
    else:
        print(RED_CODE, f"AIND holds with error = {verifier.get_error():.2} holds",
              DEFAULT_COLOR_CODE, RESET_CODE)


def get_table_df(dataset):
    df = pd.read_csv(f"examples/datasets/ind_datasets/{dataset}.csv", header=[0])
    print(f"{dataset}:", end="\n\n")
    print(df, end="\n\n")

    return df

def exact_scenario():
    print("Let's look at the datsets to consider the IND\n")

    orders_table = get_table_df('orders')
    products_table = get_table_df('products')

    algo = desbordante.aind_verification.algorithms.Default()
    algo.load_data(tables=[orders_table, products_table])
    algo.execute(lhs_indices=[2], rhs_indices=[1])

    print("Checking the IND [orders.product] -> [products.name]")
    print_results_for_ind(algo)


def approximate_scenario():
    print("-" * 80)
    print("Let's look at the datsets to consider the AIND\n")

    orders_table = get_table_df('orders')
    customers_table = get_table_df('customers')

    algo = desbordante.aind_verification.algorithms.Default()
    algo.load_data(tables=[orders_table, customers_table])

    lhs_indices = [1]
    algo.execute(lhs_indices=lhs_indices, rhs_indices=[0])

    print("Checking the error threshold for AIND [orders.customer_id] -> [customers.id]")
    print_results_for_ind(algo)

    print(f"We see, that AIND has {algo.get_error():.2} error threshold. Let's see the violating clusters.")
    print_clusters(algo, orders_table, lhs_indices)


exact_scenario()
print()
approximate_scenario()
