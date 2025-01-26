import desbordante

algo = desbordante.cind.algorithms.Default()

TABLES = [(f'../datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
          ['course', 'department']]

algo.load_data(tables=TABLES)
print(algo.time_taken())
algo.execute()
print(algo.time_taken())
algo.execute(validity=0.111, completeness=0.222, condition_type="group")
print(algo.time_taken())
algo.execute(error=0.5)
print(algo.time_taken())
inds = algo.get_ainds()
print('Found inclusion dependencies (-> means "is included in"):\n')
for ind in inds:
    print(ind)
