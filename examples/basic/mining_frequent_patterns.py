import desbordante
Database = 'examples/datasets/cmspade/cmspade_simple_test.txt'
algo = desbordante.cmspade.algorithms.CMSpade()
algo.load_data(sequence_database = Database, minsup = 0.75)
algo.execute()
result = algo.get_frequent_patterns()
print(f'Found {len(result)} frequent patterns \n')
for p in result:
    print(p)