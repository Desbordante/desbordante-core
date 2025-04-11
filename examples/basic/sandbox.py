import desbordante

algo = desbordante.cind.algorithms.Default()

TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
          ['cind_test_de', 'cind_test_en']]

algo.load_data(tables=TABLES)

algo.execute(error=0.5, validity=1, completeness=0.33, condition_type="row")
inds = algo.get_cinds()
print('Found inclusion dependencies (-> means "is included in"):\n')
for ind in inds:
    print(ind)

ind = inds[0].to_pattern_tableau()
print(ind)
print(inds[0].get_condition_attributes())




# print(algo.time_taken())
# algo.execute()
# print(algo.time_taken())
# algo.execute(error=0.5, validity=0.111, completeness=0.222, condition_type="group")
# print(algo.time_taken())
# algo.execute(error=0.5)
# print(algo.time_taken())
# inds = algo.get_ainds()
# print('Found inclusion dependencies (-> means "is included in"):\n')
# for ind in inds:
#     print(ind)
