from pathlib import Path

import desbordante
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


class bcolors:
    ARTICLE = '\033[38;2;173;255;47m'
    PERSON = '\033[38;2;46;139;87m'
    HEADER = '\033[95m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'


def colored(message, color):
    return color + message + bcolors.ENDC


GRAPH_NAME = 'papers_graph'
EMBEDDINGS_NAME = 'papers_embeddings'
GFD_NAME = 'papers_gfd'

GRAPHS_DATASETS_FOLDER_PATH = 'examples/datasets/mining_gfd'

GRAPH = Path(f'{GRAPHS_DATASETS_FOLDER_PATH}/{GRAPH_NAME}.dot')

GRAPH_IMAGE = Path(f'examples/basic/mining_gfd/figures/graphs/{GRAPH_NAME}.png')
EMBEDDINGS_IMAGE = Path(f'examples/basic/mining_gfd/figures/graphs/{EMBEDDINGS_NAME}.png')
GFD_IMAGE = Path(f'examples/basic/mining_gfd/figures/gfds/{GFD_NAME}.png')

PREAMBLE = ("Our profiler supports two tasks related to graph functional dependencies (GFDs): "
            "validation and mining (discovery). In this example, we will focus on the mining "
            "task (for validation, we refer the reader to another example). The mining algorithm "
            "used in our profiler is described in the article \"Discovering Graph Functional "
            "Dependencies\" by Fan Wenfei, Hu Chunming, Liu Xueli, and Lu Pinge, presented at SIGMOD '18.\n")

GFD_INFO = ("GFDs are functional dependencies that consist of a pattern - a graph that specifies the "
            "scope - and a rule. The nature of this object will become clearer through the "
            "example that follows.\n")

GRAPH_INFO = ("Let's analyze GFD mining through an example. Look at the graph "
              "presented on the top left in the figure. It describes the connections "
              "between scientific articles and their authors. The vertices of this "
              f"graph have two labels: {colored('Article (A)', bcolors.ARTICLE)} and "
              f"{colored('Person (P)', bcolors.PERSON)}. Each vertex has its own set "
              "of attributes depending on the label.\n\n"
              f"{colored('Article', bcolors.ARTICLE)}:\n- {colored('title', bcolors.ARTICLE)}"
              " denotes the title of the article.\n\n"
              f"{colored('Person', bcolors.PERSON)}:\n- {colored('name', bcolors.PERSON)}"
              f" denotes the name of a person,\n- {colored('role', bcolors.PERSON)}"
              " can take one of two values: \"teacher\" or \"student\".\n")

ALGO_INFO = ("The discovery algorithm, in addition to the graph, takes two parameters as input:\n"
             "- k: the maximum number of vertices in the pattern,\n"
             "- sigma: the minimum frequency of GFD occurrences in the original graph.\n")

INFO = "Let's run the algorithm and look at the result. We will set k=3 and sigma=2.\n"

REWRITING = ("It may be difficult to interpret, so let's rewrite it to a more human-readable "
             "format. Note that the empty line immediately following the colon (\":\") "
             "indicates that the left-hand side of the dependency has no conditions. "
             "Conversely, if the right-hand side of the dependency had no conditions, "
             "the second line would be empty.\n")

GFD_TEXT = (f'      {colored("0", bcolors.ARTICLE)}    {colored("1", bcolors.PERSON)}'
            f'    {colored("2", bcolors.ARTICLE)}\n'
            f'     {colored("(A)", bcolors.ARTICLE)}--{colored("(P)", bcolors.PERSON)}-'
            f'-{colored("(A)", bcolors.ARTICLE)}\n'
            '{} --> {' + colored("1", bcolors.PERSON) + '.' + colored("role", bcolors.PERSON) + ''
            '=teacher}\n\nThe mined dependency can also be seen on the right in the figure.\n')

RESULTS = ("The discovered dependency can be expressed as the following fact: If a person "
           "has two published articles, then they are a teacher.\n")

EXAMPLE_INFO = ('It is recommended to look at the second example for a deeper '
                'understanding of graph functional dependency mining. It is '
                'located in the file "mining_gfd2.py".\n')

EXIT = colored("Close the image window to finish.", bcolors.WARNING)


def execute_algo(algo):
    algo.load_data(graph=GRAPH, gfd_k=3, gfd_sigma=2)
    algo.execute()
    print(f'{bcolors.HEADER}Desbordante > {bcolors.ENDC}', end='')
    print('Mined GFDs:', len(algo.get_gfds()))
    print()
    print("Let's print found dependency (in DOT language):")
    for gfd in algo.get_gfds():
        print(gfd)
    print(REWRITING)
    print(GFD_TEXT)


def show_example():
    fig, axarr = plt.subplots(2, 2, figsize=(14, 6), gridspec_kw={'width_ratios': [7, 3], 'wspace': 0.5})
    gs = axarr[0, 1].get_gridspec()
    for ax in axarr[:, 1]:
        ax.remove()
    axsbig = fig.add_subplot(gs[:, -1])
    
    axarr[0, 0].set_axis_off()
    axarr[0, 0].set_title('$Original$ $graph$')
    axarr[0, 0].imshow(mpimg.imread(GRAPH_IMAGE))
    axarr[1, 0].set_axis_off()
    axarr[1, 0].set_title('$GFD$ $embeddings$')
    axarr[1, 0].imshow(mpimg.imread(EMBEDDINGS_IMAGE))
    axsbig.set_axis_off()
    axsbig.set_title('$Mined$ $GFD$')
    axsbig.imshow(mpimg.imread(GFD_IMAGE))
    plt.show()


print(PREAMBLE)
print(GFD_INFO)
print(GRAPH_INFO)
print(ALGO_INFO)
print(INFO)
execute_algo(desbordante.gfd_mining.algorithms.GfdMiner())
print(RESULTS)
print(EXAMPLE_INFO)
print(EXIT)

show_example()
