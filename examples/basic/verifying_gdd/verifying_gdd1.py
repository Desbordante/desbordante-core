from pathlib import Path

import matplotlib.image as mpimg
import matplotlib.pyplot as plt

import desbordante
from desbordante.gdd import AttrConst, CmpOp as Op
from desbordante.gdd import DistanceMetric as M
from desbordante.gdd import GddFromDotFile


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


PAPER_INFO = r"""This example demonstrates Graph Differential Dependency
(GDD) validation.

The pattern is defined in the paper

"Zhang, Y., Kwashie, S., Bewong, M., Hu, J., Mahboubi, A.,
Guo, X., & Feng, Z. Discovering graph differential dependencies.
Australasian Database Conference (ADC), 2023."
"""

GRAPH_INFO = r"""The displayed figure shows a small property graph (on the left).

It contains Person vertices with attributes such as "name" and "age",
and City vertices with a "name" attribute. The edge "lives_in"
connects a person to the city where that person lives.

So, informally, the picture describes several people with attributes
and the cities in which they live. This is the graph on which we will
validate our dependencies.
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
will be shown in the next example.
"""

PARAMETERS_INFO = r"""The validator receives two inputs:
1. the input graph written in DOT;
2. the list of GDDs to validate.

Each GDD consists of three parts:
1. a pattern written in DOT;
2. the left-hand side constraints;
3. the right-hand side constraints.

In this example we use only attribute-to-constant constraints.
Currently the Python API also provides relation-based helpers.
"""

GRAPH_DEFINITION = r"""A property graph is a tuple

    G = (V, E, λ, ρ)

where:
- V is the set of vertices;
- E is the set of directed edges;
- λ assigns labels to vertices and edges;
- ρ stores attribute-value pairs of vertices.

In this example, Person and City are vertex labels.
Attributes such as "name" and "age" are stored in ρ,
and "lives_in" is an edge label.
"""

PATTERN_DEFINITION = r"""A graph pattern Q[z] is a directed graph whose vertices and edges
also have labels. The list z contains all pattern vertices, that is,
all pattern variables.

Intuitively, the pattern describes the shape of subgraphs on which
the dependency is checked.

In this example the pattern is

    Person -[lives_in]-> City
"""

MATCHING_DEFINITION = r"""A match of a graph pattern in a graph is a homomorphism h from the
pattern to the graph such that:
1. each pattern vertex is mapped to a graph vertex with a matching
   label;
2. each pattern edge is mapped to a graph edge with a matching label.

Important: this is a homomorphic match, not necessarily an isomorphic
one. Distinct pattern vertices may be mapped to the same graph vertex.
This difference matters in general and will be discussed in the next
example.
"""

GDD_DEFINITION = r"""A Graph Differential Dependency has the form

    (Q[z], ΦL(X) -> ΦR(Y))

where:
- Q[z] is a graph pattern;
- ΦL(X) is the left-hand side;
- ΦR(Y) is the right-hand side;
- both ΦL(X) and ΦR(Y) are sets of distance constraints.

Let H(Q[z], G) be the set of all matches of Q[z] in graph G.
Then G satisfies the GDD iff for every match h in H(Q[z], G),

    h |= ΦL(X)  =>  h |= ΦR(Y)

So the left-hand side acts as a precondition, and the right-hand
side must hold whenever that precondition is satisfied.
"""

CONSTRAINTS_INFO = r"""In the paper, distance constraints come in six forms.

1. Attribute-to-constant:
   δ_A(x.A, c) <= t

2. Attribute-to-attribute:
   δ_{A1,A2}(x.A1, x'.A2) <= t

3. eid-to-constant:
   δ_{eid}(x.eid, ce) = 0

4. eid-to-eid:
   δ_{eid}(x.eid, x'.eid) = 0

5. Relation-to-constant:
   δ_≡(x.rela, cr) = 0

6. Relation-to-relation:
   δ_≡(x.rela, x'.rela) = 0

We usually do not use eid constraints, because the identifier of
the real-world entity is often unknown in the data. It may be
implemented later.
"""

API_INFO = r"""The current Python bindings conveniently expose these helpers:

1. AttrConst(pid, attr, const, metric, op, threshold)
   attribute-to-constant

2. AttrAttr(pid1, attr1, pid2, attr2, metric, op, threshold)
   attribute-to-attribute

3. RelConst(pid, relation, const)
   relation-to-constant

4. RelRel(pid1, relation1, pid2, relation2)
   relation-to-relation

For attribute constraints:
- pid is the pattern vertex id;
- attr is the attribute name;
- const is the compared constant, if any;
- metric is the distance metric;
- op is the comparison operator;
- threshold bounds the distance.

Desbordante version of GDD validation implements:
- EDIT_DISTANCE metric for strings;
- ABS_DIFF metric for numbers;
- LE, LT, GE, GT, EQ as the comparison operator.
"""

SHOWCASE1_INFO = r"""Showcase 1. String equality via EDIT_DISTANCE.

We validate the following rule inside the pattern (shown on the right)

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

SHOWCASE2_INFO = r"""Showcase 2. Arithmetic distance via ABS_DIFF.

Now we validate another rule on the same pattern (shown on the right
as well):

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
        AttrConst(0, "name", "Misha", M.EDIT_DISTANCE, Op.EQ, 0.0),
    ],
    rhs=[
        AttrConst(1, "name", "Amsterdam", M.EDIT_DISTANCE, Op.EQ, 0.0),
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
    ("Showcase 1", SHOWCASE1_INFO, GDD_MISHA_AMSTERDAM),
    ("Showcase 2", SHOWCASE2_INFO, GDD_UNDER_30_AMSTERDAM),
]


def show_example(graph_image_path: Path) -> None:
    img = mpimg.imread(graph_image_path)
    plt.figure(figsize=(10, 6))
    plt.axis("off")
    plt.tight_layout(pad=0)
    plt.imshow(img)
    plt.show()


def validate_gdds(gdds: list):
    algo = desbordante.gdd.Default()
    algo.load_data(graph=str(GRAPH_DOT_PATH), gdd=gdds)
    algo.execute()
    return algo.get_result()


def print_block(title: str, text: str) -> None:
    print(f"{bcolors.HEADER}{title}{bcolors.ENDC}\n")
    print(text)


def main() -> None:
    print(PAPER_INFO)

    print_block(
        "Basic definition",
        DEFINITION_INFO
    )
    print_block(
        "Parameters",
        PARAMETERS_INFO
    )
    print_block(
        "Property graph definition",
        GRAPH_DEFINITION
    )
    print_block(
        "Graph pattern definition",
        PATTERN_DEFINITION
    )
    print_block(
        "Homomorphic match definition",
        MATCHING_DEFINITION
    )
    print_block(
        "GDD syntax and semantics",
        GDD_DEFINITION
    )
    print_block(
        "Six forms of distance constraints",
        CONSTRAINTS_INFO
    )
    print_block(
        "How these constraints are represented in Python",
        API_INFO
    )

    print_block(
        "Dataset",
        GRAPH_INFO
    )

    gdds = [gdd for _, _, gdd in SHOWCASES]
    valid_gdds = validate_gdds(gdds)

    for i, (title, info, gdd) in enumerate(SHOWCASES):
        print(f"{bcolors.HEADER}{title}{bcolors.ENDC}\n")
        print(info)

        print(f"{bcolors.HEADER}Desbordante > {bcolors.ENDC}", end="")
        if gdd in valid_gdds:
            print("GDD holds.\n")
        else:
            print("GDD does not hold.\n")

    print_block(
        "What we learned",
        OUTRO_INFO
    )

    print(f"{bcolors.WARNING}Close the image window to finish.{bcolors.ENDC}")
    show_example(IMAGE_PATH)


if __name__ == "__main__":
    main()
