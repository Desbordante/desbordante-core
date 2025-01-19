import desbordante

# algo = desbordante.cind.algorithms.Default()

TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
          ['course', 'department']]

# algo.load_data(tables=TABLES)
# algo.execute()
# inds = algo.get_inds()
# print('Found inclusion dependencies (-> means "is included in"):\n')
# for ind in inds:
#     print(ind)
