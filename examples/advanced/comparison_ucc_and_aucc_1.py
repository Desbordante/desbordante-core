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

TABLE = 'examples/datasets/ucc_datasets/ucc_aucc_1.csv'
CORRECT_TABLE = 'examples/datasets/ucc_datasets/ucc_aucc_1_correct.csv'

print(f'''{CYAN}This example illustrates the difference between exact and approximate Unique
Column Combinations (UCC and AUCC). Intuitively, a UCC declares that some columns uniquely
identify every tuple in a table. An AUCC allows a certain degree of violation. For more
information on UCC, consult "A Hybrid Approach for Efficient Unique Column Combination Discovery"
by T. Papenbrock and F. Naumann. For more information on AUCC, consult "Efficient Discovery of
Approximate Dependencies" by S. Kruse and F. Naumann.
{ENDC}''')

print('The following table contains records about employees:')
print(CYAN, end='')
print_table(TABLE)
print(ENDC, end='')
print('We\'ll try to find typos, using UCC mining and AUCC verifying algorithms.\n')

print('Let\'s run UCC mining algorithm:')
algo = desbordante.ucc.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))
algo.execute()

uccs = algo.get_uccs()
print('Found UCCs:')
for ucc in uccs:
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()

print(f'There is no UCC for {CYAN}Name{ENDC} column. Maybe it\'s an error?')
print(f'Let\'s run AUCC verification algorithm for column {CYAN}Name{ENDC}:')
ver_algo = desbordante.aucc_verification.algorithms.Default()
ver_algo.load_data(table=(TABLE, ',', True))
ver_algo.execute(ucc_indices=[0])

if ver_algo.ucc_holds():
    print(f'{GREEN}UCC holds{ENDC}')
else:
    print(f'{RED}UCC does not hold{ENDC}, but AUCC holds with threshold = {ver_algo.get_error():1.3f}')

print('Threshold is small. It means that, possibly, there is an error in this column.')
print('Let\'s look at the table again:')
print(CYAN, end='')
print_table(TABLE)
print(ENDC)

print(f'''There are two {CYAN}Harrys{ENDC}. They have different work experience, so they are
two different employees. If they had unique names, AUCC would hold with threshold = 0, and
{CYAN}Name{ENDC} could be used as a key:''')
print(CYAN, end='')
print_table(CORRECT_TABLE)
print(ENDC)

print('Let\'s run UCC mining algorithm:')
algo = desbordante.ucc.algorithms.Default()
algo.load_data(table=(CORRECT_TABLE, ',', True))
algo.execute()

uccs = algo.get_uccs()
print('Found UCCs:')
for ucc in uccs:
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()

print(f'Now we have cleaned the data and {CYAN}Name{ENDC} can now be used as a key.')
