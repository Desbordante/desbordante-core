import desbordante

GRAPH = 'examples/datasets/fsm/gspan_test_large.txt'

algo = desbordante.gspan.algorithms.GSpan()
algo.load_data(graph_database=GRAPH, minsup = 0.5, output_single_graph=False, max_number_of_edges=42)
algo.execute()
result = algo.get_frequent_subgraphs()
print(len(result), 'frequent subgraphs found with minsup = 0.5:\n')
