from pathlib import Path

import desbordante
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


class bcolors:
    STUDENT = '\033[38;2;254;136;99m'
    TASK = '\033[38;2;87;206;235m'
    HEADER = '\033[95m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'


def colored(message, color):
    return color + message + bcolors.ENDC


GRAPH_NAME = 'study_graph'
EMBEDDINGS_NAME = 'study_embeddings'
GFD_NAME = 'study_gfd'

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
              "between students and tasks. The vertices of this "
              f"graph have two labels: {colored('Student (S)', bcolors.STUDENT)} and "
              f"{colored('Task (T)', bcolors.TASK)}. Each vertex has its own set "
              "of attributes depending on the label.\n\n"
              f"{colored('Student', bcolors.STUDENT)}:\n- {colored('name', bcolors.STUDENT)}"
              f" denotes the name of the student,\n- {colored('degree', bcolors.STUDENT)} "
              f"is the level of education,\n- {colored('year', bcolors.STUDENT)} "
              "is the year of study.\n\n"
              f"{colored('Task', bcolors.TASK)}:\n- {colored('name', bcolors.TASK)}"
              f" denotes the name of a task,\n- {colored('difficulty', bcolors.TASK)}"
              " is a categorical parameter that takes one of the following values: "
              "\"easy\", \"normal\" or \"hard\".\n")

ALGO_INFO = ("The discovery algorithm, in addition to the graph, takes two parameters as input:\n"
             "- k: the maximum number of vertices in the pattern,\n"
             "- sigma: the minimum frequency of GFD occurrences in the original graph.\n")

INFO = "Let's run the algorithm and look at the result. We will set k=2 and sigma=3.\n"

REWRITING = ("It may be difficult to interpret, so let's rewrite it to a more human-readable "
             "format. Notation: the first line contains the literals found in the left-hand side. "
             "The second line contains those in the right-hand side.\n")

GFD_TEXT = ('                      '
            f' {colored("0", bcolors.TASK)}    {colored("1", bcolors.STUDENT)}\n'
            '                      '
            f'{colored("(T)", bcolors.TASK)}--{colored("(S)", bcolors.STUDENT)}\n'
            '{' + colored("0", bcolors.TASK) + '.' + colored("difficulty", bcolors.TASK) + ''
            '=hard} --> {' + colored("1", bcolors.STUDENT) + '.'
            '' + colored("degree", bcolors.STUDENT) + '=master & ' + colored("1", bcolors.STUDENT) + ''
            '.' + colored("year", bcolors.STUDENT) + '=2}\n\nThe mined dependency can also be '\
            'seen on the right in the figure.\n')

RESULTS = ("The dependency found indicates that only second-year master's "
           "students are working on the difficult task.\n")

EXAMPLE_INFO = ('It is recommended to look at the first example for a deeper '
                'understanding of graph functional dependency mining. It is '
                'located in the file "mining_gfd1.py".\n')

EXIT = colored("Close the image window to finish.", bcolors.WARNING)


def execute_algo(algo):
    algo.load_data(graph=GRAPH, gfd_k=2, gfd_sigma=3)
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
    fig, axarr = plt.subplots(2, 2, figsize=(16, 7), gridspec_kw={'width_ratios': [7, 3], 'wspace': 0.5})
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
