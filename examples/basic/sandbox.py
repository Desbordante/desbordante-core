import desbordante

def check(algo1, algo2, cond_type):
    inds1 = algo1.get_cinds()
    inds2 = algo2.get_cinds()
    assert len(inds1) == len(inds2)
    for i in range(len(inds1)):
        attrs1 = inds1[i].get_condition_attributes()
        attrs2 = inds2[i].get_condition_attributes()
        assert attrs1 == attrs2

        conds1 = inds1[i].to_pattern_tableau()
        conds2 = inds2[i].to_pattern_tableau()
        if (set(conds1) != set(conds2)):
            print(i, "\n")
            print(inds1[i], "\n")
            print(inds2[i], "\n")
        assert set(conds1) == set(conds2)
        print(cond_type + ":", "ind", i, "completed")
    print()
    

algo1 = desbordante.cind.algorithms.Default()
algo2 = desbordante.cind.algorithms.Default()

TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
          ['cind_test_de', 'cind_test_en']]

algo1.load_data(tables=TABLES,algo_type="cinderella")
algo2.load_data(tables=TABLES,algo_type="pli_cind")

for cond_type in ["row", "group"]:
    algo1.execute(error=0.5, validity=1, completeness=0.33, condition_type=cond_type)
    algo2.execute(error=0.5, validity=1, completeness=0.33, condition_type=cond_type)

    check(algo1, algo2, cond_type)


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
