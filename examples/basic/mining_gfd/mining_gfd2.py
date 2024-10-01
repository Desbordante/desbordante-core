from pathlib import Path

import desbordante
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


class bcolors:
    HEADER = '\033[95m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'


GRAPH_NAME = 'study_graph'
GFD_NAME = 'study_gfd'

GRAPHS_DATASETS_FOLDER_PATH = 'examples/datasets/mining_gfd'

GRAPH = Path(f'{GRAPHS_DATASETS_FOLDER_PATH}/{GRAPH_NAME}.dot')

GRAPH_IMAGE = Path(f'examples/basic/mining_gfd/figures/graphs/{GRAPH_NAME}.png')
GFD_IMAGE = Path(f'examples/basic/mining_gfd/figures/gfds/{GFD_NAME}.png')

GRAPH_INFO = ('Figure provides an example of a graph. '
              'The following abbreviations were used here: T - task, S - student. '
              'The vertices with the T-label have the attributes "name" and "difficulty"'
              ', the vertices with the S-label have the "name", "degree" and "year" '
              'attributes, which indicate the student\'s name, level of education and year. '
              'The values of these attributes are signed next to the vertices, except for '
              'the name, since it is not informative.\n')

INFO = ("Let's run the algorithm. We'll specify 2 as the k parameter to look for patterns "
        "with no more than two vertices, and we'll specify 3 as the sigma to exclude "
        "rare dependencies.\n")

RESULTS = ("The dependency found indicates that only second-year master's students are "
           "working on the hard task.\n")

EXIT = f'{bcolors.WARNING}Close the image window to finish.{bcolors.ENDC}'


def execute_algo(algo):
    algo.load_data(graph=GRAPH, gfd_k=2, gfd_sigma=3)
    algo.execute()
    print(f'{bcolors.HEADER}Desbordante > {bcolors.ENDC}', end='')
    print('Mined GFDs:', len(algo.get_gfds()))
    print()


def show_example():
    _, axarr = plt.subplots(1, 2, figsize=(12, 5), gridspec_kw={'width_ratios': [7, 3], 'wspace': 0.5}) 
    axarr[0].set_axis_off()
    axarr[0].set_title('$Graph$')
    axarr[0].imshow(mpimg.imread(GRAPH_IMAGE))
    axarr[1].set_axis_off()
    axarr[1].set_title('$Mined$ $GFD$')
    axarr[1].imshow(mpimg.imread(GFD_IMAGE))
    plt.show()


print(GRAPH_INFO)
print(INFO)
execute_algo(desbordante.gfd_mining.algorithms.GfdMiner())
print(RESULTS)
print(EXIT)

show_example()
