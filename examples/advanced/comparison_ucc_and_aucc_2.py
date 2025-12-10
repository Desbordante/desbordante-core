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

TABLE = 'examples/datasets/ucc_datasets/ucc_aucc_2.csv'
CORRECT_TABLE = 'examples/datasets/ucc_datasets/ucc_aucc_2_correct.csv'

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
print('We\'ll try to find typos, using UCC and AUCC mining algorithms.\n')

print('Let\'s run AUCC mining algorithm with threshold = 0.013:')
a_algo = desbordante.ucc.algorithms.PyroUCC()
a_algo.load_data(table=(TABLE, ',', True))
a_algo.execute(error=0.013)

auccs = a_algo.get_uccs()
print('Found AUCCs:')
for aucc in sorted(auccs, key=lambda x: x.to_long_string()):
    print(f'\t{CYAN}{aucc.to_long_string()}{ENDC}')
print()

print('Let\'s run UCC mining algorithm:')
e_algo = desbordante.ucc.algorithms.Default()
e_algo.load_data(table=(TABLE, ',', True))
e_algo.execute()

uccs = e_algo.get_uccs()
print('Found UCCs:')
for ucc in sorted(uccs, key=lambda x: x.to_long_string()):
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()

print('Now let\'s find AUCCs that are not UCCs -- these columns may contain errors:')
only_approximate = set(auccs) - set(uccs)
print('ACCs - UCCs =')
for ucc in sorted(only_approximate, key=lambda x: x.to_long_string()):
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()

print(f'''{CYAN}[Last_name Grade Salary]{ENDC} has cardinality of 3 -- it\'s "accidental" UCC.
{CYAN}[First_name Grade]{ENDC} and {CYAN}[First_name Salary]{ENDC} may not hold -- people with the
same first name may have same grade or salary. But {CYAN}[First_name Last_name]{ENDC} must
hold -- even if two employees have same names, their records in our table should be uniquely
identifiable by {CYAN}[First_name Last_name]{ENDC} pair. Let\'s look at the table again:''')
print(CYAN, end='')
print_table(TABLE)
print(ENDC)

print(f'''There are two {CYAN}Allens{ENDC} without the last name and two {CYAN}Dorothy
Weawer's{ENDC}. All they have different experience, therefore all of them are different
employees. Thus, it is an oversight or typo in the table. Let\'s improve the quality of this data:''')
print(CYAN, end='')
print_table(CORRECT_TABLE)
print(ENDC)

print(f'Now UCC {CYAN}[First_name Last_name]{ENDC} should hold. Let\'s run UCC mining algorithm again:')
e_algo = desbordante.ucc.algorithms.Default()
e_algo.load_data(table=(CORRECT_TABLE, ',', True))
e_algo.execute()

uccs = e_algo.get_uccs()
print('Found UCCs:')
for ucc in sorted(uccs, key=lambda x: x.to_long_string()):
    print(f'\t{CYAN}{ucc.to_long_string()}{ENDC}')
print()
print(f'''UCC {CYAN}[First_name Last_name]{ENDC} holds, and we have found and resolved two
inconsistencies in the data.''')
