from pathlib import Path

import desbordante
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


class bcolors:
    HEADER = '\033[95m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'


GRAPH_NAME = 'blogs_graph'
MODIFIED_GRAPH_NAME = 'two_authors_blogs_graph'
GFD_NAME = 'one_author_gfd'

GRAPHS_DATASETS_FOLDER_PATH = 'examples/datasets/verifying_gfd/graphs'
GFDS_DATASETS_FOLDER_PATH = 'examples/datasets/verifying_gfd/gfds'

GRAPH = Path(f'{GRAPHS_DATASETS_FOLDER_PATH}/{GRAPH_NAME}.dot')
MODIFIED_GRAPH = Path(f'{GRAPHS_DATASETS_FOLDER_PATH}/{MODIFIED_GRAPH_NAME}.dot')
GFD = Path(f'{GFDS_DATASETS_FOLDER_PATH}/{GFD_NAME}.dot')

GRAPHS_FIGURES_FOLDER_PATH = 'examples/basic/verifying_gfd/figures/graphs'
GFDS_FIGURES_FOLDER_PATH = 'examples/basic/verifying_gfd/figures/gfds'

GRAPH_IMAGE = Path(f'{GRAPHS_FIGURES_FOLDER_PATH}/{GRAPH_NAME}.png')
MODIFIED_GRAPH_IMAGE = Path(f'{GRAPHS_FIGURES_FOLDER_PATH}/{MODIFIED_GRAPH_NAME}.png')
GFD_IMAGE = Path(f'{GFDS_FIGURES_FOLDER_PATH}/{GFD_NAME}.png')

GRAPH_INFO = ('The graph is depicted in the figure. The following abbreviations '
              'were used: A - account, B - blog. Vertices labeled A have a '
              '"name" attribute showing the nickname; vertices labeled B - '
              '"author", indicating who wrote the blog. The values of these '
              'attributes are labeled next to the vertices. The edges are also '
              'labeled as: "post", which indicates who wrote the blog, and '
              '"like", which indicates approval by another person. In the '
              'drawing, the edges are marked "post" in bold.\n')

GFD_INFO = ('The dependency on the figure suggests that one blog cannot have '
            'two authors. That is, satisfiability of this dependency ensures '
            'that there are no errors related to the number of authors in '
            'the data.\n')

INFO = "Let's check if this dependency holds.\n"

RESULTS = 'Well, GFD is really satisfied as expected.\n'

CONTINUE = f'{bcolors.WARNING}Close the image window to continue.{bcolors.ENDC}\n'

MODIFIED_GRAPH_INFO = ('Let\'s now modify the graph to see how the algorithm '
                       'will behave in another case. The new graph is depicted '
                       'in the new figure. Replaced edge label between the '
                       'first left blog and Donatello account from '
                       '"like" to "post".\n')

MODIFIED_INFO = 'Run algorithm:\n'

MODIFIED_RESULTS = ('As you can see, the modified graph does not satisfy this '
                    'dependency, indicating that it has errors.\n')

EXIT = f'{bcolors.WARNING}Close the image window to finish.{bcolors.ENDC}'


def execute_algo(algo, graph):
    algo.load_data(gfd=[GFD], graph=graph)
    algo.execute()
    print(f'{bcolors.HEADER}Desbordante > {bcolors.ENDC}', end='')
    if (len(algo.get_gfds()) == 1):
        print('GFD holds.\n')
    else:
        print('GFD does not hold.\n')


def show_example(title, graph_path):
    _, axarr = plt.subplots(1, 2, figsize=(12, 5), gridspec_kw={'width_ratios': [7, 2], 'wspace': 0.5}) 
    axarr[0].set_axis_off()
    axarr[0].set_title(title)
    axarr[0].imshow(mpimg.imread(graph_path))
    axarr[1].set_axis_off()
    axarr[1].set_title('$GFD$')
    axarr[1].imshow(mpimg.imread(GFD_IMAGE))
    axarr[1].text(x=-75, y=300, s=r'$\{\varnothing \rightarrow 1.name=2.name\}$', fontsize=14)
    plt.show()


print(GRAPH_INFO)
print(GFD_INFO)
print(INFO)
execute_algo(desbordante.gfd_verification.algorithms.EGfdValid(), GRAPH)
print(RESULTS)
print(CONTINUE)

show_example('$Graph$', GRAPH_IMAGE)

print(MODIFIED_GRAPH_INFO)
print(MODIFIED_INFO)
execute_algo(desbordante.gfd_verification.algorithms.EGfdValid(), MODIFIED_GRAPH)
print(MODIFIED_RESULTS)
print(EXIT)

show_example('$Modified$ $graph$', MODIFIED_GRAPH_IMAGE)
