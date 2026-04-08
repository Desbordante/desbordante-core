from pathlib import Path
import matplotlib.image as mpimg
import matplotlib.pyplot as plt

import desbordante
from desbordante.gdd import AttrConst, CmpOp as Op, DistanceMetric as M, GddFromDotFile


class bcolors:
    HEADER = "\033[95m"
    WARNING = "\033[93m"
    ENDC = "\033[0m"


GRAPH_NAME = "people_cities_graph"
PATTERN_NAME = "misha_lives_in_amsterdam_pattern"

GRAPHS_DATASETS_DIR = Path(r"examples/datasets/verifying_gdd/graphs")
GDDS_DATASETS_DIR = Path(r"examples/datasets/verifying_gdd/gdds")

GRAPH_DOT_PATH = GRAPHS_DATASETS_DIR / f"{GRAPH_NAME}.dot"
PATTERN_DOT_PATH = GDDS_DATASETS_DIR / f"{PATTERN_NAME}.dot"

FIGURES_DIR = Path(r"examples/basic/verifying_gdd/figures")
IMAGE_PATH = FIGURES_DIR / "people_cities.png"


GRAPH_INFO = r"""The figure shows a small property graph.

It contains Person vertices with attributes such as "name" and "age",
and City vertices with a "name" attribute. The edge "lives_in"
connects a person to the city where that person lives.

So, informally, the picture describes several people with attributes
and the cities in which they live. This is the graph on which we will
validate our dependencies.
"""

PAPER_INFO = r"""This example demonstrates Graph Differential Dependency
(GDD) validation.

The primitive is defined in the paper

"Zhang, Y., Kwashie, S., Bewong, M., Hu, J., Mahboubi, A.,
Guo, X., & Feng, Z. Discovering graph differential dependencies.
Australasian Database Conference (ADC), 2023."
"""

DEFINITION_INFO = r"""A Graph Differential Dependency has the form

    (Q[z], ΦL(X) -> ΦR(Y))

Here Q[z] is a graph pattern, while ΦL(X) and ΦR(Y) are sets of
distance constraints over the variables of that pattern.

Semantically, a GDD states the following: for every homomorphic match
of the pattern in the graph, if all constraints from the left-hand
side hold, then all constraints from the right-hand side must also
hold.

In simpler terms, a GDD is a formal implication checked on all
homomorphic matches of the pattern.

The difference between a homomorphic match and an isomorphic match
will be explained in the next example.
"""

PARAMETERS_INFO = r"""The validator receives two inputs:
1. the input graph written in DOT;
2. the list of GDDs to validate.

Each GDD consists of three parts:
1. a pattern written in DOT;
2. the left-hand side constraints;
3. the right-hand side constraints.

Each constraint specifies:
1. the pattern vertex id;
2. the attribute name;
3. the compared constant value;
4. the distance metric;
5. the comparison operator;
6. the threshold.
"""

SHOWCASE_1_INFO = r"""Showcase 1. String equality via EDIT_DISTANCE.

We validate the following rule inside the pattern

    Person -[lives_in]-> City

The concrete dependency says:

    if 0.name = "Misha", then 1.name = "Amsterdam"

Since EDIT_DISTANCE with threshold 0.0 means exact string equality,
this is simply the implication

    "Misha" -> "Amsterdam"

This dependency is expected not to hold, because not every matched
person with name "Misha" lives in Amsterdam. One of the two Mishas
lives in Riga.
"""

SHOWCASE_2_INFO = r"""Showcase 2. Arithmetic distance via ABS_DIFF.

Now we validate another rule on the same pattern:

    if 0.age < 30, then 1.name = "Amsterdam"

With the current API, ABS_DIFF expresses absolute distance to a
constant. Therefore, assuming non-negative ages, we encode

    0.age < 30

as

    |0.age - 0| <= 29

This is a convenient showcase for ABS_DIFF in this dataset.
This dependency is expected to hold because, in this graph, every
matched person younger than 30 lives in Amsterdam. Misha and Bob
are 25, while the other Misha is 31 and lives in Riga.
"""

OUTRO_INFO = r"""In this example we learned how to describe a graph pattern,
attach distance constraints to its vertices, and validate several GDDs
in a single validator run on one graph.

We also saw two kinds of constraints in practice:
exact string equality through EDIT_DISTANCE, and numeric comparison
through ABS_DIFF.

For a more realistic scenario based on fact checking, read the example

    verifying_gdd2.py
"""


GDD_MISHA_AMSTERDAM = GddFromDotFile(
    pattern_dot_file=PATTERN_DOT_PATH,
    lhs=[
        AttrConst(0, "name", "Misha", M.EDIT_DISTANCE, Op.LE, 0.0),
    ],
    rhs=[
        AttrConst(1, "name", "Amsterdam", M.EDIT_DISTANCE, Op.LE, 0.0),
    ],
)

GDD_UNDER_30_AMSTERDAM = GddFromDotFile(
    pattern_dot_file=PATTERN_DOT_PATH,
    lhs=[
        AttrConst(0, "age", 0, M.ABS_DIFF, Op.LE, 29.0),
    ],
    rhs=[
        AttrConst(1, "name", "Amsterdam", M.EDIT_DISTANCE, Op.LE, 0.0),
    ],
)

SHOWCASES = [
    ("Showcase 1", SHOWCASE_1_INFO, GDD_MISHA_AMSTERDAM),
    ("Showcase 2", SHOWCASE_2_INFO, GDD_UNDER_30_AMSTERDAM),
]


def show_example(graph_image_path: Path) -> None:
    img = mpimg.imread(graph_image_path)
    plt.figure(figsize=(10, 6))
    plt.axis("off")
    plt.tight_layout(pad=0)
    plt.imshow(img)
    plt.show()


def validate_gdds(gdds: list) -> list:
    algo = desbordante.gdd.Default()
    algo.load_data(graph=str(GRAPH_DOT_PATH), gdd=gdds)
    algo.execute()
    return algo.get_result()


def main() -> None:
    print(PAPER_INFO)
    print(DEFINITION_INFO)
    print(PARAMETERS_INFO)
    print(GRAPH_INFO)

    gdds = [gdd for _, _, gdd in SHOWCASES]
    valid_gdds = validate_gdds(gdds)

    for title, info, gdd in SHOWCASES:
        print(f"{bcolors.HEADER}{title}{bcolors.ENDC}\n")
        print(info)

        print(f"{bcolors.HEADER}Desbordante > {bcolors.ENDC}", end="")
        if gdd in valid_gdds:
            print("GDD holds.\n")
        else:
            print("GDD does not hold.\n")

    print(OUTRO_INFO)
    print(f"{bcolors.WARNING}Close the image window to finish.{bcolors.ENDC}")
    show_example(IMAGE_PATH)


if __name__ == "__main__":
    main()
