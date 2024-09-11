import desbordante
from csv import reader

RED = '\033[31m'
GREEN = '\033[32m'
CYAN = '\033[1m\033[36m'
ENDC = '\033[0m'

def row_to_padded_string(row: list[str], widths: list[int]) -> str:
    return ''.join(field.ljust(width) for width, field in zip(widths, row))

def print_table(filename: str) -> None:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))

    column_widths = []
    for col_num in range(len(rows[0])):
        max_len = max(len(row[col_num]) for row in rows)
        column_widths.append(max_len + 3)

    header, *data_rows = rows
    print(row_to_padded_string(header, column_widths))
    print('-' * sum(column_widths))
    print('\n'.join(row_to_padded_string(row, column_widths) for row in data_rows))

TABLE = 'examples/datasets/ucc_datasets/ucc.csv'

print(f'''{CYAN}This example illustrates the usage of exact Unique Column Combinations (UCC).
Intuitively, a UCC declares that some columns uniquely identify every tuple in a table.
For more information consult "A Hybrid Approach for Efficient Unique Column Combination Discovery"
by T. Papenbrock and F. Naumann.
{ENDC}''')

print('The following table contains records about employees:')
print(CYAN, end='')
print_table(TABLE)
print(ENDC, end='')
print('We need to select a column or a combination of columns that will serve as a unique key (ID).\n')

print('Let\'s run UCC mining algorithm:')
algo = desbordante.ucc.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))
algo.execute()

uccs = algo.get_uccs()
print('Found UCCs:')
for ucc in uccs:
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()

print('There are no unary UCCs, so there is no single column that can define a key.')
print('We need to select a combination of two columns, that will serve as an ID.')
print(f'{CYAN}[First_name Last_name]{ENDC} is a good candidate.')
