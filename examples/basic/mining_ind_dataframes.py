import csv

import desbordante
import pandas as pd


def make_named_df(name):
    df = pd.read_csv(f'examples/datasets/ind_datasets/{name}.csv', sep=',', header=0)
    df.attrs['name'] = name
    return df


TABLES = [make_named_df(table_name) for table_name in
          ['course', 'department', 'instructor', 'student', 'teaches']]

algo = desbordante.ind.algorithms.Default()
algo.load_data(tables=TABLES)
algo.execute()
inds = algo.get_inds()
print('Found inclusion dependencies (-> means "is included in"):\n')
for ind in inds:
    print(ind)
print()

print('Tables for first IND:')
print()
first_ind = inds[0]
for i in first_ind.get_lhs().table_index, first_ind.get_rhs().table_index:
    df = TABLES[i]
    print(df.attrs['name'])
    print(TABLES[i])
    print()
