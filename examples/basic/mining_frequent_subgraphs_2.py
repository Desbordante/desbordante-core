from pathlib import Path
try:
    import matplotlib.pyplot as plt
    import networkx as nx
    HAS_VISUALIZATION = True
except ImportError:
    HAS_VISUALIZATION = False
import desbordante

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def colored(text, color):
    return f"{color}{text}{bcolors.ENDC}"


GRAPH_DATABASE_FILE = 'examples/datasets/fsm/gspan_test_simple.txt'


MINSUP = 0.9


INTRO_TEXT = (
    "In this example, we will explore the gSpan algorithm (Graph-Based Substructure Pattern Mining).\n"
    "gSpan is an algorithm for mining frequent subgraphs in a graph database.\n\n"
    "A graph database consists of a collection of graphs. The goal of gSpan is to find all subgraphs \n"
    "that appear in at least a certain percentage of the graphs in the database. This percentage is \n"
    "called minimum support (minsup).\n\n"
)

PARAM_INFO = (
    "The algorithm accepts the following options:\n"
    "- minsup: The minimum support threshold. It is a value between 0 and 1.\n"
    "  For example, a minsup of 0.6 means a subgraph must appear in at least 60% of the graphs \n"
    "  in the database to be considered 'frequent'.\n"
    "- output_single_vertices: (Optional, default=True) If set to False, frequent subgraphs containing only a single vertex will NOT be output.\n"
    "- max_number_of_edges: (Optional) Limits the maximum number of edges in the mined subgraphs.\n"
    "  Useful for focusing on smaller patterns or improving performance.\n"
    "- output_graph_ids: (Optional) Output the ids of graph containing each frequent subgraph.\n"
    "- output_path: (Optional) Path to output file for frequent subgraphs (if empty, no file is written).\n\n"
)

EXECUTION_INFO = (
    "We will now run the gSpan algorithm on a sample dataset containing a small collection of graphs.\n"
    f"For this demonstration, we set minsup = {MINSUP}, meaning we are only interested in patterns \n"
    f"that appear in almost all graphs (at least {int(MINSUP*100)}%).\n\n"
)

RESULT_INFO = (
    "The algorithm has finished execution and identified the frequent subgraphs.\n"
    "Each listed result represents a subgraph pattern that meets our stringent support criteria.\n\n"
)


ANALYSIS_TEXT = (
    "Having reviewed the raw output, let's break down exactly why these specific patterns were flagged:\n\n"
    "1. Pattern #1 (single vertex with label '11'):\n"
    "   - Graph #0 has nodes with label '11' (vertices 1 and 3).\n"
    "   - Graph #1 has a node with label '11' (vertex 5).\n"
    "   - Graph #2 has nodes with label '11' (vertices 8 and 9).\n"
    "   -> Present in 3/3 graphs (100%), so meets minsup 90%.\n\n"
    "2. Pattern #2 (single vertex with label '10'):\n"
    "   - Present in all three graphs as well.\n\n"
    "3. Pattern #3 (edge 10 --[20]-- 11):\n"
    "   - Graph #0: Edge between node 0 (label 10) and 1 (label 11) has label 20.\n"
    "   - Graph #1: Edge between node 4 (label 10) and 5 (label 11) has label 20.\n"
    "   - Graph #2: Edge between node 7 (label 10) and 9 (label 11) has label 20.\n"
    "   -> This specific configuration of labels appears in all graphs.\n\n"
    "Note on other edges:\n"
    "- Edge (11)--[23]--(10) appears in Graph #0 and Graph #2, but NOT in Graph #1.\n"
    "  Support is 2/3 (66%), which is less than minsup (90%), so it is not frequent."
)


def print_section(title, content):
    print(colored(f"\n=== {title} ===", bcolors.HEADER))
    print(content)


def visualize_demo_graphs():
    if not HAS_VISUALIZATION:
        print(colored("Skipping visualization: matplotlib or networkx not found.", bcolors.WARNING))
        return

    print(colored("\nVisualizing Graphs...", bcolors.HEADER))

    input_graphs = []

    # Graph 0
    G0 = nx.Graph()
    G0.add_node(0, label='10'); G0.add_node(1, label='11'); G0.add_node(2, label='10'); G0.add_node(3, label='11')
    G0.add_edge(0, 1, label='20'); G0.add_edge(1, 2, label='23'); G0.add_edge(1, 3, label='22')
    G0.name = "Input #0"
    input_graphs.append(G0)

    # Graph 1
    G1 = nx.Graph()
    G1.add_node(4, label='10'); G1.add_node(5, label='11')
    G1.add_edge(4, 5, label='20')
    G1.name = "Input #1"
    input_graphs.append(G1)

    # Graph 2
    G2 = nx.Graph()
    G2.add_node(6, label='10'); G2.add_node(7, label='10'); G2.add_node(8, label='11'); G2.add_node(9, label='11')
    G2.add_edge(6, 7, label='21'); G2.add_edge(7, 8, label='23'); G2.add_edge(7, 9, label='20'); G2.add_edge(8, 9, label='22')
    G2.name = "Input #2"
    input_graphs.append(G2)

    # --- Result Graphs ---
    result_graphs = []

    # Result 1: v 11
    R1 = nx.Graph()
    R1.add_node(0, label='11')
    R1.name = "Pattern #1"
    result_graphs.append(R1)

    # Result 2: v 10
    R2 = nx.Graph()
    R2.add_node(0, label='10')
    R2.name = "Pattern #2"
    result_graphs.append(R2)

    # Result 3: v 10 --(20)-- v 11
    R3 = nx.Graph()
    R3.add_node(0, label='10'); R3.add_node(1, label='11')
    R3.add_edge(0, 1, label='20')
    R3.name = "Pattern #3"
    result_graphs.append(R3)

    all_graphs = input_graphs + result_graphs
    n = len(all_graphs)
    cols = 3
    rows = (n + cols - 1) // cols
    
    plt.figure(figsize=(4 * cols, 4 * rows))
    
    for i, G in enumerate(all_graphs):
        ax = plt.subplot(rows, cols, i + 1)
        pos = nx.spring_layout(G, seed=42)
        
        node_labels = nx.get_node_attributes(G, 'label')
        edge_labels = nx.get_edge_attributes(G, 'label')
        
        nx.draw(G, pos, ax=ax, with_labels=True, labels=node_labels, 
                node_color='lightgreen', node_size=800, font_weight='bold')
        nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, ax=ax)
        
        ax.set_title(G.name)
        ax.axis('off')

    plt.tight_layout()
    print(colored("Opening visualization window...", bcolors.OKCYAN))
    plt.show()


def main():
    print_section("Introduction", INTRO_TEXT)
    
    print_section("Parameters", PARAM_INFO)

    print_section("Execution", EXECUTION_INFO)
    
    print(colored(f"Loading data from: {GRAPH_DATABASE_FILE}", bcolors.OKCYAN))
    
    algo = desbordante.gspan.algorithms.GSpan()
    
    algo.load_data(
        # The path to the graph database file
        graph_database=GRAPH_DATABASE_FILE, 
        
        # Minimum support threshold
        minsup=MINSUP, 
        
        # Set to False to exclude single-vertex patterns
        output_single_vertices=True,
        
        # Optional: Limit the size of subgraphs to mine
        # max_number_of_edges=42 
    )
    
    print(colored("Running gSpan algorithm...", bcolors.OKBLUE))
    algo.execute()
    print(colored("Done!", bcolors.OKGREEN))

    print_section("Results", RESULT_INFO)
    
    results = algo.get_frequent_subgraphs()
    print(f"Number of frequent subgraphs found: {colored(len(results), bcolors.BOLD)}")
    
    if len(results) > 0:
        print("\nHere are the found frequent subgraphs:")
        print(colored("Format Legend:", bcolors.HEADER))
        print("  t # <id> * <support>: Graph ID and support count")
        print("  v <vertex_id> <label>: Vertex definition")
        print("  e <v_id1> <v_id2> <label>: Edge definition")
        print("  x <graph_ids...>: List of graph IDs containing this pattern")
        
        for i, subgraph in enumerate(results):
            print(colored(f"\n--- Subgraph #{i+1} ---", bcolors.OKCYAN))
            print(subgraph)

    print_section("Analysis", ANALYSIS_TEXT)

    visualize_demo_graphs()



if __name__ == "__main__":
    main()
