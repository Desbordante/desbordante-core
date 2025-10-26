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
    

# algo1 = desbordante.cind.algorithms.Default()
# algo2 = desbordante.cind.algorithms.Default()
# algo3 = desbordante.cind.algorithms.Default()
# algo4 = desbordante.cind.algorithms.Default()

# TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
#           ['anime_prep', 'manga_prep']]
# TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
#           ['names_prep', 'states_prep']]
TABLES = [(f'examples/datasets/ind_datasets/{table_name}.csv', ',', True) for table_name in
          ['cind_test_de', 'cind_test_en']]

# algo1.load_data(tables=TABLES,algo_type="cinderella")
# algo2.load_data(tables=TABLES,algo_type="cinderella")
# algo3.load_data(tables=TABLES,algo_type="pli_cind")
# algo4.load_data(tables=TABLES,algo_type="pli_cind")
# VALIDITY = 0.8
VALIDITY = 0.95
# COMPLETENESS = 0.005
COMPLETENESS = 0.05


# algo1.execute(error=0.5, validity=VALIDITY, completeness=COMPLETENESS, condition_type="row")
# algo2.execute(error=0.5, validity=VALIDITY, completeness=COMPLETENESS, condition_type="group")
# algo3.execute(error=0.5, validity=VALIDITY, completeness=COMPLETENESS, condition_type="row")
# algo4.execute(error=0.5, validity=VALIDITY, completeness=COMPLETENESS, condition_type="group")

# for cond_type in [
#     "row",
#     # "group"
#     ]:
#     algo1.execute(error=0.5, validity=VALIDITY, completeness=COMPLETENESS, condition_type=cond_type)
#     algo2.execute(error=0.5, validity=VALIDITY, completeness=COMPLETENESS, condition_type=cond_type)
#     check(algo1, algo2, cond_type)

import desbordante

# TABLES = ['examples/datasets/ind_datasets/cind_test_de.csv', 'examples/datasets/ind_datasets/cind_test_en.csv']

algo = desbordante.cind.algorithms.Default()
algo.load_data(tables=TABLES,algo_type="cinderella")
algo.execute(error=0.5, validity=0.75, completeness=0.25, condition_type="row")
for cind in algo.get_cinds():
    print(cind)
    if (cind.conditions_number()):
        condition = cind.get_conditions()[0]
        print(f"First condition metrics: validity = {condition.precision()}, completeness = {condition.recall()}") 
