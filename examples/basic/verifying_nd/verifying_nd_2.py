import desbordante
from csv import reader

RED = '\033[31m'
GREEN = '\033[32m'
CYAN = '\033[1m\033[36m'
ENDC = '\033[0m'

def row_to_padded_string(row: list[str], widths: list[int]) -> str:
    return ''.join(field.ljust(width) for width, field in zip(widths, row))

def print_table(filename: str, row_numbers: list[int] = []) -> None:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))

    needed_rows = [rows[0]]
    if len(row_numbers) > 0:
        for row_num in row_numbers:
            needed_rows.append(rows[row_num + 1])
    else:
        needed_rows = rows

    column_widths = []
    for col_num in range(len(needed_rows[0])):
        max_len = max(len(row[col_num]) for row in needed_rows)
        column_widths.append(max_len + 3)

    header, *data_rows = needed_rows
    print(row_to_padded_string(header, column_widths))
    print('-' * sum(column_widths))
    print('\n'.join(row_to_padded_string(row, column_widths) for row in data_rows))

MERGED_PEOPLE_TABLE = 'examples/datasets/nd_verification_datasets/merged_people.csv'
VALID_PASSPORTS_TABLE = 'examples/datasets/nd_verification_datasets/valid_passports_2.csv'

print(f'''{CYAN}This example illustrates the usage of Numerical Dependencies (NDs).
Intuitively, given two sets of attributes X and Y, there is an ND from X to Y (denoted X -> Y, weight = k)
if each value of X can never be associated to more than k distinct values of Y.
For more information consult "Efficient derivation of numerical dependencies" by Paolo Ciaccia et al.
{ENDC}''')

print('Citizens of Arstozka can have no more than two documents: one Arstozka passport and one exit permit.')
print('The following table contains records of some citizens\' documents:')

print(CYAN, end='')
print_table(MERGED_PEOPLE_TABLE)
print(ENDC)

algo = desbordante.nd_verification.algorithms.NDVerifier()
algo.load_data(table=(MERGED_PEOPLE_TABLE, ',', True))
algo.execute(lhs_indices=[0], rhs_indices=[1], weight=2)

highlights = algo.highlights

print('We need to validate these data')
print('Let\'s run ND verification algorithm to check that every citizen has no more than two records:')
print('\tND: {Name} -> {ID} with weight 2')
print(f'\tND holds: {RED}{algo.nd_holds}{ENDC}')
print(f'\tActual weight is {algo.real_weight}')
print()

print('Let\'s look at clusters violating ND:')
print(f'Number of clusters: {len(highlights)}')
highlight_indices = []
for high in highlights:
    highlight_indices += high.get_occurences_indices()

print(CYAN, end='')
print_table(MERGED_PEOPLE_TABLE, highlight_indices)
print(ENDC)
print(f'So, {highlights[0].lhs_value} has {highlights[0].occurences_number} documents. It\'s twice as much as needed.')
print(f'Look at birth date. {highlights[0].lhs_value} has two different values.')
print(f'Maybe, we have two different {highlights[0].lhs_value}? Let\'s split them:')
print(CYAN, end='')
print_table(VALID_PASSPORTS_TABLE)
print(ENDC)
print('Let\'s run algorithm again:')

algo = desbordante.nd_verification.algorithms.NDVerifier()
algo.load_data(table=(VALID_PASSPORTS_TABLE, ',', True))
algo.execute(lhs_indices=[0], rhs_indices=[1], weight=2)

print(f'\tND holds: {GREEN}{algo.nd_holds}{ENDC}')
