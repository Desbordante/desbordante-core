import desbordante

GRAPH = 'examples/datasets/fsm/gspan_test_simple.txt'

algo = desbordante.gspan.algorithms.GSpan()
algo.load_data(graph_database=GRAPH, minsup = 0.9)
algo.execute()
result = algo.get_frequent_subgraphs()
print(len(result), 'frequent subgraphs found with minsup = 0.9:\n')
print('Frequent Subgraphs:')
for sg in result:
    print(sg)