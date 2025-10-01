import desbordante

print("""
    ========================================================================
    This example demonstrates Desbordante"s ability to process
    conditional inclusion dependencies (CIND"s) discovery algorithm.
    
    We consider CIND as approximate inclusion dependency (AIND), which 
    scope defined by conditions set over one or more attributes. These
    conditions utilizes the precision and recall metrics to represent their
    validity (condition matches only included data) and completeness
    (condition matches all included data) respectively. Also, there are 2
    types of conditions: "row" condition counts metrics for data tuples,
    and "group" condition counts them for sets of entries, grouped by their
    inclusion attributes. 
    
    Given algorithm discovers every CIND, whose conditions has given type
    and condition metrics are not worse, than user-specified thresholds.
    
    For more information about CIND, consider: 
    Covering or complete? : discovering conditional inclusion dependencies
    by J. Bauckmann, Z. Abedjan, U. Leser, H. Müller and F. Naumann
    ========================================================================
    """)

algo = desbordante.cind.algorithms.Default()

TABLES = [(f"examples/datasets/ind_datasets/{table_name}.csv", ",", True) for table_name in
          ["cind_test_de", "cind_test_en"]]
algo.load_data(tables=TABLES)

ERROR_THRESHOLD = 0.5 # AIND error threshold, since CIND is an AIND
CONDITION_TYPE = "row"
VALIDITY_THRESHOLD = 0.75
COMPLETENESS_THRESHOLD = 0.25

algo.execute(error=ERROR_THRESHOLD, validity=VALIDITY_THRESHOLD, completeness=COMPLETENESS_THRESHOLD, condition_type=CONDITION_TYPE)

print('Found condotoinal inclusion dependencies (-> means "is included in"):')
for cind in algo.get_cinds():
    print(cind)

print("CIND's and their conditions, that found by algorithm, are also a valid python objects")
cind = algo.get_cinds()[0]
print()
print("Example CIND object:")
print(cind)
print("CIND object has methods:")
print("    get_condition_attributes: ", cind.get_condition_attributes())
print("    conditions_number: ", cind.conditions_number())
print("    get_conditions (get all found conditions as Condition objects)")

condition = cind.get_conditions()[0]
print()
print("Example Condition object:")
print(condition)
print("Condition object has methods:")
print("    data: ", condition.data())
print('    validity (also might be "precision"): ', condition.validity())
assert condition.validity() == condition.precision()
print('    completeness (also might be "recall"): ', condition.completeness())
assert condition.completeness() == condition.recall()

print("Also, CIND and Condition objects have __str__(), __eq__(), __hash__() methods")
