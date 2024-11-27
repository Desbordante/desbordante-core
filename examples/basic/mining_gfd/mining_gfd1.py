from pathlib import Path

import desbordante
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


class bcolors:
    HEADER = '\033[95m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'


GRAPH_NAME = 'blogs_graph'
GFD_NAME = 'blogs_gfd'

GRAPHS_DATASETS_FOLDER_PATH = 'examples/datasets/mining_gfd'

GRAPH = Path(f'{GRAPHS_DATASETS_FOLDER_PATH}/{GRAPH_NAME}.dot')

GRAPH_IMAGE = Path(f'examples/basic/mining_gfd/figures/graphs/{GRAPH_NAME}.png')
GFD_IMAGE = Path(f'examples/basic/mining_gfd/figures/gfds/{GFD_NAME}.png')

GRAPH_INFO = ('The graph is depicted in figure. The following abbreviations '
              'were used: A - account, B - blog. Vertices labeled A have a '
              '"name" attribute showing the nickname; vertices labeled B - '
              '"author", indicating who wrote the blog. The values of these '
              'attributes are labeled next to the vertices. The edges are also '
              'labeled as: "post", which indicates who wrote the blog, and '
              '"like", which indicates approval by another person. In the '
              'drawing, the edges are marked "post" in bold.\n')

INFO = ("Let's run the algorithm and look at the result. We will consider "
        "all dependencies with a pattern of no more than 3 vertices, as well as "
        "with a frequency of occurrence of at least 3 times.\n")

RESULTS = ("The found dependency indicates that if the author has posted a "
           "blog, then the authorship of this blog always includes the "
           "name of the person who posted it.\n")

EXIT = f'{bcolors.WARNING}Close the image window to finish.{bcolors.ENDC}'


def execute_algo(algo):
    algo.load_data(graph=GRAPH, gfd_k=3, gfd_sigma=3)
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
