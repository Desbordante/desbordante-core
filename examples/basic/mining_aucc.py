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

TABLE = 'examples/datasets/ucc_datasets/aucc.csv'
CORRECT_TABLE = 'examples/datasets/ucc_datasets/aucc_correct.csv'

print(f'''{CYAN}This example illustrates the usage of approximate Unique Column Combinations
(AUCC). Intuitively, an AUCC declares that some columns uniquely identify every tuple in a table,
but allows a certain degree of violation. For more information consult "Efficient Discovery of
Approximate Dependencies" by S. Kruse and F. Naumann.
{ENDC}''')

print('The following table contains records about employees:')
print(CYAN, end='')
print_table(TABLE)
print(ENDC, end='')
print('We need to select a column that will serve as a unique key (ID).\n')
print('''The AUCC mining algorithm with different error threshold will be used. The smaller
threshold gets, the less violations (repeated values) are allowed in column combinations.
Setting threshold to 0 means mining exact UCCs (without violations).''')

print('Let\'s run AUCC mining algorithm with threshold = 0:')
algo = desbordante.ucc.algorithms.PyroUCC()
algo.load_data(table=(TABLE, ',', True))
algo.execute(error=0)

uccs = algo.get_uccs()
print('Found UCCs:')
for ucc in uccs:
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()

print('There are no unary UCCs, so there is no single column that can define a key.')
print('Let\'s run algorithm with bigger threshold (= 0.1):')
algo = desbordante.ucc.algorithms.PyroUCC()
algo.load_data(table=(TABLE, ',', True))
algo.execute(error=0.1)

auccs = algo.get_uccs()
print('Found AUCCs:')
for aucc in auccs:
    print(f'\t{CYAN}{aucc.to_long_string()}{ENDC}')
print()

print('Now, almost all columns are considered to be unique. It\'s not what we wanted.')
print('Let\'s select a smaller threshold (= 0.05):')
algo = desbordante.ucc.algorithms.PyroUCC()
algo.load_data(table=(TABLE, ',', True))
algo.execute(error=0.05)

auccs = algo.get_uccs()
print('Found AUCCs:')
for aucc in auccs:
    print(f'\t{CYAN}{aucc.to_long_string()}{ENDC}')
print()

print(f'''Out of single-column UCCs, {CYAN}Name{ENDC} requires the smallest threshold to be "unique".
It means that {CYAN}Name{ENDC} has less violations than other columns.
Let\'s look at the table again, paying a special attention to the {CYAN}Name{ENDC} column:''')
print(CYAN, end='')
print_table(TABLE)
print(ENDC)

print(f'''There are two {CYAN}Harrys{ENDC}. They have different work experience and salary,
therefore they are two different employees. This is most likely an error/oversight in data.
If we represented their records using unique names, the {CYAN}Name{ENDC} AUCC would hold with
threshold = 0, and {CYAN}Name{ENDC} could be used as a key:''')
print(CYAN, end='')
print_table(CORRECT_TABLE)
print(ENDC)

print('Let\'s run algorithm once more with threshold = 0:')
algo = desbordante.ucc.algorithms.PyroUCC()
algo.load_data(table=(CORRECT_TABLE, ',', True))
algo.execute(error=0)

uccs = algo.get_uccs()
print('Found UCCs:')
for ucc in uccs:
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print(f'Now we can use {CYAN}Name{ENDC} as a key')
