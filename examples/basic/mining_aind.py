import desbordante
import pandas as pd
from tabulate import tabulate

print('''==============================================
In Desbordante, we consider an approximate inclusion dependency (AIND)
as any inclusion dependency (IND) that utilizes an error metric to measure
violations. This metric calculates the proportion of distinct values in the
dependent set (LHS) that must be removed to satisfy the dependency on the
referenced set (RHS) completely.

The metric lies within the [0, 1] range:
- A value of 0 means the IND holds exactly (no violations exist).
- A value closer to 1 indicates a significant proportion of LHS values violate
      the dependency.

Desbordante supports the discovery and verification of both exact INDs and AINDs:
1) Exact INDs: All values in the LHS set must match a value in the RHS set.
2) Approximate INDs (AINDs): Allows for controlled violations quantified by the
error metric.

For discovery tasks, users can specify an error threshold, and Desbordante will
return all AINDs with an error value equal to or less than the specified
threshold.

The error metric used for AINDs in Desbordante is an adaptation of g3,
originally designed for approximate functional dependencies (FDs).

For more information, consider:
1) Unary and n-ary inclusion dependency discovery in relational databases by
   Fabien De Marchi, Stéphane Lopes, and Jean-Marc Petit.
==============================================''')


def print_table(table, title):
    print(title)
    print(tabulate(table, headers='keys', tablefmt='psql'), end='\n\n')


def get_table_path(dataset):
    return f"examples/datasets/ind_datasets/{dataset}.csv"


TABLE_NAMES = ['employee', 'project_assignments']
TABLES = [(get_table_path(name), ',', True) for name in TABLE_NAMES]

print()
print('Now, we are going to demonstrate how to discover AINDs.')
print()

print("""The datasets under consideration for this example are
'employee' and 'project_assignments'.""")
print()

for table_index in range(len(TABLE_NAMES)):
    table_name = TABLE_NAMES[table_index]
    df = pd.read_csv(get_table_path(table_name), header=[0])
    print_table(df, f"Dataset '{table_name}':")

ERROR_THRESHOLD = 0.3
print(f"Let's find all AINDs with an error threshold less than {ERROR_THRESHOLD}.")

algo = desbordante.ind.algorithms.Mind()
algo.load_data(tables=TABLES)
algo.execute(error=ERROR_THRESHOLD)

print()
print('Found inclusion dependencies (-> means "is included in"):')
for ind in algo.get_inds():
    print("IND:", ind)

print()
print('''We found only a single AIND, this dependency contains typos in the
"Employee Name" column of the second dataset.''')
print()
print('''For automatically detecting violating clusters, you can create a
pipeline using the AIND verifier in combination with a mining algorithm.

For an additional example, refer to the examples/advanced/aind_typos.py''')
