import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"
RESET_CODE = "\033[0m"


def prints(str):
    print(textwrap.fill(str, 80))


def print_table(table, title):
    print(title)
    print(tabulate(table, headers='keys', tablefmt='psql'), end='\n\n')


def print_clusters(verifier, table, indices):
    if verifier.get_error() == 0:
        print(GREEN_CODE, "IND holds", RESET_CODE)
    else:
        print(RED_CODE, f"AIND holds with error = {verifier.get_error():.2}",
              RESET_CODE)

    print(f"Number of clusters violating IND: {verifier.get_violating_clusters_count()}")
    for i, cluster in enumerate(verifier.get_violating_clusters(), start=1):
        print(f"{BLUE_CODE} #{i} cluster: {DEFAULT_COLOR_CODE}{RESET_CODE}")
        for el in cluster:
            values = " ".join([f"{table[table.columns[idx]][el]}" for idx in indices])
            print(f"{el}: {values}")
    print()


def verify_aind(lhs, rhs):
    (lhs_col, lhs_indices) = lhs.to_index_tuple()
    (rhs_col, rhs_indices) = rhs.to_index_tuple()
    lhs_table = TABLE_CONFIGS[lhs_col]
    rhs_table = TABLE_CONFIGS[rhs_col]

    verifier = desbordante.aind_verification.algorithms.Default()
    verifier.load_data(tables=[lhs_table, rhs_table])
    verifier.execute(lhs_indices=lhs_indices, rhs_indices=rhs_indices)
    print_clusters(verifier, TABLE_DFS[lhs_col], lhs_indices)


def get_table_path(dataset):
    return f"examples/datasets/ind_datasets/{dataset}.csv"


prints("""This pipeline demonstrates the process of mining and verifying AINDs
(approximate inclusion dependencies). This pipeline can be used for data cleaning
by identifying typos in the datasets based on the identified AINDs.""")

print()
prints("It consists of the following steps:")
prints("1. Mine all possible AINDs from a set of tables.")
prints("2. Filter out exact INDs (which have zero error).")
prints("3. Verify the AINDs to identify clusters of data that violate the dependencies.")
prints("4. Display detailed information about the dependencies.")
print()

TABLE_NAMES = ['orders', 'customers', 'products']
# We will use those configs for dependency mining
TABLE_CONFIGS = [(get_table_path(name), ',', True) for name in TABLE_NAMES]
# Pandas dataframe will be used to show clusters.
TABLE_DFS = [pd.read_csv(get_table_path(name), header=[0]) for name in TABLE_NAMES]
# It is important to note that searching for exact dependencies takes significantly
# less time and resources. To search for exact dependencies, it is preferable to use
# the Faida algorithm, which is a much more efficient algorithm for searching for
# exact dependencies.

ERROR_THRESHOLD = 0.4
prints(f"Let's find all AINDs with an error threshold less than {ERROR_THRESHOLD}.")
prints("""The datasets under consideration for this scenario are
 'orders', 'customers' and 'products'.""")
print()

for table_index in range(len(TABLE_NAMES)):
    print_table(TABLE_DFS[table_index], f"Dataset '{TABLE_NAMES[table_index]}':")

algo = desbordante.aind.algorithms.Mind()
algo.load_data(tables=TABLE_CONFIGS)
algo.execute(error=ERROR_THRESHOLD)

inds = algo.get_inds()

print("Here is the list of exact INDs:")
exact_inds = list(filter(lambda i: i.get_error() == 0.0, inds))
for ind in exact_inds:
    print(ind)

print()

print("Here is the list of AINDs:")
typo_ainds = list(filter(lambda i: i.get_error() != 0.0, inds))

for typo_aind in typo_ainds:
    print("AIND:", typo_aind)

print()
print("Let's see detailed information about AINDs:")

# For each AIND with an error, verify and print clusters
# with potential typos:
for typo_aind in typo_ainds:
    print('AIND:', typo_aind)
    verify_aind(typo_aind.get_lhs(), typo_aind.get_rhs())

prints("""Based on the analysis of the AINDs and their errors, we
can make the following conclusions:""")
print()

prints("""  First AIND:
The AIND between `orders.customer_id` and `customers.id` holds with
an error threshold of 0.33. The clusters violating the inclusion
dependency indicate possible data issues.""")

prints("""- The `orders.customer_id` value '201' does not match any
entry in the `customers.id` column. This suggests that there might
have been a typo where '201' should have been '101', indicating that
the customer who bought the 'Mouse' might actually be Alice.""")

prints("""- Similarly, the `orders.customer_id` value '108' also
violates the AIND. This suggests a missing customer entry in the
`customers` table. The customer corresponding to '108' may not
exist in the dataset, pointing to a data inconsistency.""")

print()
prints("""  Second AIND:
The AIND between `customers.id` and `orders.customer_id` with an
error threshold of 0.2 does not indicate any real typo. The issue here
is that this AIND counts customers who have not placed any orders,
which is expected behavior. This dependency does not reflect typos
but instead reveals missing orders for certain customers. Since it's
not a typo, this AIND is not useful for cleaning data.""")

print()
prints("""It's important to take the error threshold into account when
working with AINDs. A higher threshold will reveal more potential errors,
but it might also include non-typo cases, such as customers who have
not made any orders yet.""")
