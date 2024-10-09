import desbordante
import pandas as pd


GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"
RESET_CODE = "\033[0m"


def get_table_path(dataset):
    return f"examples/datasets/ind_datasets/{dataset}.csv"


def get_table_df(dataset):
    df = pd.read_csv(get_table_path(dataset), header=[0])
    print(f"{dataset}:", end="\n\n")
    print(df, end="\n\n")
    return df


def print_results_for_ind(verifier):
    if verifier.get_error() == 0:
        print(GREEN_CODE, "IND holds", DEFAULT_COLOR_CODE, RESET_CODE)
    else:
        print(RED_CODE, f"AIND holds with error = {verifier.get_error():.2} holds",
              DEFAULT_COLOR_CODE, RESET_CODE)


def print_clusters(verifier, table, indices):
    print_results_for_ind(verifier)
    print(f"Number of clusters violating IND: {len(verifier.get_violating_clusters())}")
    for i, cluster in enumerate(verifier.get_violating_clusters(), start=1):
        print(f"{BLUE_CODE} #{i} cluster: {DEFAULT_COLOR_CODE}{RESET_CODE}")
        for el in cluster:
            values = " ".join([f"{table[table.columns[idx]][el]}" for idx in indices])
            print(f"{el}: {values}")


def verify_aind(lhs, rhs):
    (lhs_col, lhs_indices) = lhs
    (rhs_col, rhs_indices) = rhs
    lhs_table = TABLE_CONFIGS[lhs_col]
    rhs_table = TABLE_CONFIGS[rhs_col]

    verifier = desbordante.aind_verification.algorithms.Default()
    verifier.load_data(tables=[lhs_table, rhs_table])
    verifier.execute(lhs_indices=lhs_indices, rhs_indices=rhs_indices)
    print_clusters(verifier, TABLE_DFS[lhs_col], lhs_indices)


TABLE_NAMES = ['orders', 'customers', 'products']
TABLE_DFS= [get_table_df(path) for path in TABLE_NAMES]
TABLE_CONFIGS= [(get_table_path(name), ',', True) for name in TABLE_NAMES]
ERROR = 0.4

print(f"Let\'s find all INDs with error less than {ERROR}.", end="\n\n")

algo = desbordante.aind.algorithms.Default()
algo.load_data(tables=TABLE_CONFIGS)
algo.execute(error=ERROR)

inds = algo.get_inds()

print("There are list of exact INDs:")
exact_inds = list(filter(lambda i: i.get_error() == 0.0, inds))
for ind in exact_inds:
    print(ind)

print()

print("There are list of AINDs:")
ainds = list(filter(lambda i: i.get_error() != 0.0, inds))

for aind in ainds:
    print("AIND:", aind)

print("\nLet's see detailed information about AINDs")

for aind in ainds:
    print("\nAIND:", aind)
    verify_aind(aind.get_lhs().to_index_tuple(), aind.get_rhs().to_index_tuple())


