import desbordante
import csv

def row_to_padded_string(row, widths):
    return ''.join(field.ljust(width) for width, field in zip(widths, row))

def print_table(filename: str):
    with open(filename, newline='') as table:
        rows = list(csv.reader(table, delimiter=','))
    
    column_widths = []
    for col_num in range(len(rows[0])):
        max_len = max(len(row[col_num]) for row in rows)
        column_widths.append(max_len + 3)

    header, *data_rows = rows
    print(row_to_padded_string(header, column_widths))
    print('-' * sum(column_widths))
    print('\n'.join(row_to_padded_string(row, column_widths) for row in data_rows))

TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
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
print('course.csv:\n')
print_table('examples/datasets/ind_datasets/course.csv')

print()
print('department.csv:\n')
print_table('examples/datasets/ind_datasets/department.csv')
