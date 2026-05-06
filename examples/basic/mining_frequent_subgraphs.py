try:
    import matplotlib.patches as mpatches
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


def print_section(title, content):
    print(colored(f"\n=== {title} ===", bcolors.HEADER))
    print(content)


GRAPH_DATABASE_FILE = 'examples/datasets/fsm/gspan_test_simple.txt'

MINSUP = 0.9


INTRO_TEXT = (
    "In this example, we will explore the gSpan algorithm (Graph-Based Substructure Pattern Mining).\n"
    "This algorithm is defined in the following paper:\n"
    "Xifeng Yan and Jiawei Han. gSpan: Graph-Based Substructure Pattern Mining.\n"
    "IEEE International Conference on Data Mining, ICDM'02. 721-724. 10.1109/ICDM.2002.1184038.\n\n"
    "Suppose you have a large collection of graphs — molecules, social networks, or program flow graphs — and you want\n"
    "to find common substructures that appear frequently across this collection. This is exactly what Frequent\n"
    "Subgraph Mining (FSM) does.\n\n"
    "Unlike verification primitives that check if a specific rule holds (like Functional Dependencies), FSM is a\n"
    "discovery primitive. It searches the entire search space to find all subgraphs that appear in at least a\n"
    "minimum percentage of the graphs in your database. This percentage is called 'minimum support' (minsup).\n\n"
    "For instance, in chemoinformatics, vertices might represent atoms and edges chemical bonds.\n"
    "FSM can automatically discover common functional groups (like a benzene ring) that appear in many compounds.\n\n"
    "It is important to note that gSpan operates on graphs with numeric labels on both vertices and edges.\n"
    "The algorithm considers two vertices to be identical only if they share the same numeric label.\n"
    "Similarly, edges are matched based on their numeric labels and the labels of the vertices they connect.\n\n"
    "gSpan is an exact algorithm, meaning it guarantees finding ALL subgraphs that meet the criteria."
)

PARAM_INFO = (
    "When running gSpan, the most critical parameter is `minsup` (minimum support).\n"
    "This value, ranging from 0 to 1, sets the threshold for what counts as 'frequent'.\n"
    "For example, a minsup of 0.6 requires a pattern to be present in 60% of the graphs in your database.\n\n"
    "You can also fine-tune the output:\n"
    "  - output_single_vertices=False  — exclude single-vertex patterns from results\n"
    "  - max_number_of_edges=N         — limit discovered patterns to at most N edges\n"
    "  - output_graph_ids=True         — include IDs of graphs containing each pattern"
)

ANALYSIS_TEXT = (
    "Why did we find these specific patterns? Let's take a closer look.\n"
    "With our high support threshold of 90%, a pattern must appear in all 3 graphs in our dataset.\n\n"
    "First, we see single vertices with labels '10' and '11'. This is expected —\n"
    "every graph contains at least one node with each of these labels.\n\n"
    "More interestingly, we found the edge pattern `10 --[20]-- 11`:\n"
    "  - Graph #0: edge between node 0 (label 10) and node 1 (label 11), edge label 20\n"
    "  - Graph #1: edge between node 4 (label 10) and node 5 (label 11), edge label 20\n"
    "  - Graph #2: edge between node 7 (label 10) and node 9 (label 11), edge label 20\n\n"
    "Other edges, like `11 --[23]-- 10`, appear in Graph #0 and Graph #2, but are missing from Graph #1.\n"
    "Their support is 66%, which is below our 90% threshold — so they didn't make the cut."
)


def _draw_cell(ax, G, node_color, face_color, border_color, fontsize=11):
    pos = nx.spring_layout(G, seed=42)
    node_labels = nx.get_node_attributes(G, 'label')
    edge_labels = nx.get_edge_attributes(G, 'label')
    nx.draw(G, pos, ax=ax, with_labels=True, labels=node_labels,
            node_color=node_color, node_size=800, font_weight='bold')
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, ax=ax)
    ax.patch.set_visible(True)
    ax.patch.set_facecolor(face_color)
    ax.add_patch(mpatches.FancyBboxPatch(
        (0, 0), 1, 1, boxstyle='square,pad=0', transform=ax.transAxes,
        fill=False, edgecolor=border_color, linewidth=2.5, clip_on=False))
    ax.text(0.5, -0.05, G.name, transform=ax.transAxes,
            ha='center', va='top', fontsize=fontsize,
            fontweight='bold', color=border_color)


def visualize_demo_graphs():
    if not HAS_VISUALIZATION:
        print(colored("Skipping visualization: matplotlib or networkx not found.", bcolors.WARNING))
        return

    print(colored("\nVisualizing Input Graphs and Discovered Patterns...", bcolors.HEADER))

    # Input graphs
    G0 = nx.Graph()
    G0.add_node(0, label='10'); G0.add_node(1, label='11')
    G0.add_node(2, label='10'); G0.add_node(3, label='11')
    G0.add_edge(0, 1, label='20'); G0.add_edge(1, 2, label='23'); G0.add_edge(1, 3, label='22')
    G0.name = "Input Graph #0"

    G1 = nx.Graph()
    G1.add_node(4, label='10'); G1.add_node(5, label='11')
    G1.add_edge(4, 5, label='20')
    G1.name = "Input Graph #1"

    G2 = nx.Graph()
    G2.add_node(6, label='10'); G2.add_node(7, label='10')
    G2.add_node(8, label='11'); G2.add_node(9, label='11')
    G2.add_edge(6, 7, label='21'); G2.add_edge(7, 8, label='23')
    G2.add_edge(7, 9, label='20'); G2.add_edge(8, 9, label='22')
    G2.name = "Input Graph #2"

    # Frequent patterns at minsup=0.9
    R1 = nx.Graph()
    R1.add_node(0, label='11')
    R1.name = "Pattern #1"

    R2 = nx.Graph()
    R2.add_node(0, label='10')
    R2.name = "Pattern #2"

    R3 = nx.Graph()
    R3.add_node(0, label='10'); R3.add_node(1, label='11')
    R3.add_edge(0, 1, label='20')
    R3.name = "Pattern #3"

    # 6 tiles: top row = input graphs, bottom row = patterns
    fig, axes = plt.subplots(2, 3, figsize=(14, 9))
    fig.patch.set_facecolor('#f0f0f0')
    plt.subplots_adjust(top=0.90, bottom=0.10, hspace=0.60, wspace=0.30, left=0.04, right=0.96)

    fig.text(0.5, 0.95, 'Input Graph Database', ha='center', va='top',
             fontsize=13, fontweight='bold', color='steelblue')

    fig.add_artist(plt.Line2D([0.03, 0.97], [0.50, 0.50],
                              transform=fig.transFigure, color='#888888',
                              linewidth=1.5, linestyle='--'))

    fig.text(0.5, 0.49, f'Frequent Patterns   (minsup = {MINSUP})', ha='center', va='top',
             fontsize=13, fontweight='bold', color='seagreen')

    for i, G in enumerate([G0, G1, G2]):
        _draw_cell(axes[0, i], G, 'lightsteelblue', 'aliceblue', 'steelblue')

    for i, G in enumerate([R1, R2, R3]):
        _draw_cell(axes[1, i], G, 'lightgreen', 'honeydew', 'seagreen')

    print(colored("Opening visualization window...", bcolors.OKCYAN))
    plt.show()


def subgraph_to_nx(subgraph, index):
    G = nx.Graph()

    labels = subgraph.edge_list.get_vertex_labels()
    for vid, label in enumerate(labels):
        G.add_node(int(vid), label=str(label))

    for edge in subgraph.edge_list:
        v1_id = int(edge.vertex1.id)
        v2_id = int(edge.vertex2.id)
        v1_label = str(edge.vertex1.label)
        v2_label = str(edge.vertex2.label)

        if not G.has_node(v1_id):
            G.add_node(v1_id, label=v1_label)
        if not G.has_node(v2_id):
            G.add_node(v2_id, label=v2_label)

        if int(edge.label) != -1:
            G.add_edge(v1_id, v2_id, label=str(edge.label))

    G.name = f"Pattern #{index + 1}  (support: {subgraph.support})"
    return G


def print_subgraph_components(subgraph, index):
    print(colored(f"\n  Pattern #{index + 1}", bcolors.OKCYAN)
          + f"  support={colored(subgraph.support, bcolors.BOLD)}"
          + f"  graphs={sorted(subgraph.graphs_ids)}")

    vertices = {}
    for edge in subgraph.edge_list:
        vertices[int(edge.vertex1.id)] = edge.vertex1.label
        vertices[int(edge.vertex2.id)] = edge.vertex2.label
    if vertices:
        print(colored("  Vertices:", bcolors.OKBLUE))
        for vid in sorted(vertices):
            print(f"    v{vid}  label={vertices[vid]}")

    edges = [edge for edge in subgraph.edge_list if int(edge.label) != -1]
    if edges:
        print(colored("  Edges:", bcolors.OKBLUE))
        for edge in edges:
            print(f"    v{edge.vertex1.id}(label={edge.vertex1.label})"
                  f"  --[{edge.label}]--"
                  f"  v{edge.vertex2.id}(label={edge.vertex2.label})")
    else:
        print(colored("  (single-vertex pattern, no edges)", bcolors.WARNING))


def visualize_subgraph_results(results, title, node_color='lightsalmon',
                               face_color='seashell', border_color='sienna'):
    if not HAS_VISUALIZATION:
        return

    graphs = [subgraph_to_nx(r, i) for i, r in enumerate(results)]
    if not graphs:
        return

    cols = min(3, len(graphs))
    rows = (len(graphs) + cols - 1) // cols

    fig, axes_arr = plt.subplots(rows, cols, figsize=(4.5 * cols, 4.5 * rows + 0.5), squeeze=False)
    fig.suptitle(title, fontsize=13, fontweight='bold')
    fig.patch.set_facecolor('#f5f5f5')
    plt.subplots_adjust(hspace=0.60, wspace=0.30, bottom=0.08, top=0.92)

    for i, G in enumerate(graphs):
        _draw_cell(axes_arr[i // cols][i % cols], G, node_color, face_color, border_color, fontsize=10)

    for i in range(len(graphs), rows * cols):
        axes_arr[i // cols][i % cols].set_visible(False)

    print(colored("Opening visualization window...", bcolors.OKCYAN))
    plt.show()


def main():
    # Intro + dataset format
    print_section("Introduction", INTRO_TEXT)
    print_section("Parameters", PARAM_INFO)

    print_section("Dataset", "We will use the following dataset for this example:")
    print(colored(f"  File: {GRAPH_DATABASE_FILE}", bcolors.OKCYAN))
    print("\nDataset format:")
    print("  t # N          — graph with ID N")
    print("  v ID LABEL     — vertex")
    print("  e ID1 ID2 LABEL — edge between two vertices")

    # Configure and run gSpan with the initial minsup
    print_section("Execution",
                  f"Running gSpan with minsup = {MINSUP} "
                  f"(patterns must appear in at least {int(MINSUP * 100)}% of graphs).")

    algo = desbordante.gspan.algorithms.GSpan()
    algo.load_data(
        graph_database=GRAPH_DATABASE_FILE,
        minsup=MINSUP,
        output_single_vertices=True,
        # max_number_of_edges=42,  # optional: limit pattern size
    )

    print(colored("  Running...", bcolors.OKBLUE))
    algo.execute()
    print(colored("  Done!", bcolors.OKGREEN))

    results = algo.get_frequent_subgraphs()

    # Inspect results in two forms: raw text and structured access
    print_section("Results (raw format)",
                  f"Found {colored(len(results), bcolors.BOLD)} frequent subgraph(s).\n\n"
                  "Format legend:\n"
                  "  t # <id> * <support>     — pattern ID and support count\n"
                  "  v <id> <label>           — vertex\n"
                  "  e <v1> <v2> <label>      — edge\n"
                  "  x <graph_ids...>         — graphs containing this pattern")

    for i, subgraph in enumerate(results):
        print(colored(f"\n--- Subgraph #{i + 1} ---", bcolors.OKCYAN))
        print(str(subgraph).strip())

    print_section("Results (structured access)",
                  "Each FrequentSubgraph has a .edge_list field.\n"
                  "You can iterate over it to inspect edges and see vertex IDs/labels and edge labels.")

    for i, subgraph in enumerate(results):
        print_subgraph_components(subgraph, i)

    # Human-readable interpretation + demo visualization
    print_section("Analysis", ANALYSIS_TEXT)

    visualize_demo_graphs()

    # Re-run with lower minsup on the same algorithm object
    LOWER_MINSUP = 0.6
    print_section("Behavior Change: Lowering Support",
                  f"Let's re-run with minsup = {LOWER_MINSUP} to see how many more patterns emerge.")

    print(colored(f"  Running with minsup = {LOWER_MINSUP}...", bcolors.OKBLUE))
    algo.execute(minsup=LOWER_MINSUP, output_single_vertices=True)
    results_low = algo.get_frequent_subgraphs()
    print(colored("  Done!", bcolors.OKGREEN))

    new_patterns = [r for r in results_low if r not in results]
    print(f"\n  Found {colored(len(results_low), bcolors.BOLD)} patterns "
          f"(+{len(new_patterns)} compared to minsup={MINSUP}).")


    print(f"\nNew patterns (showing up to 3 of {len(new_patterns)}):")
    for r in new_patterns[:3]:
        print(colored("\n  --- New Pattern ---", bcolors.OKCYAN))
        print(str(r).strip())
    if len(new_patterns) > 3:
        print("  ...")

    print(colored("\nVisualizing all patterns found at the lower support threshold...", bcolors.HEADER))
    visualize_subgraph_results(
        results_low,
        f"All Frequent Patterns  (minsup = {LOWER_MINSUP})",
    )

    print_section("Conclusion",
                  "You have now learned the fundamentals of Frequent Subgraph Mining with the gSpan algorithm.\n"
                  "Feel free to experiment with different datasets and minsup values.")



if __name__ == "__main__":
    main()
