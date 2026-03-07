from pathlib import Path
import tempfile

import matplotlib.image as mpimg
import matplotlib.pyplot as plt

import desbordante
from desbordante.gdd import AttrAttr, CmpOp as Op
from desbordante.gdd import DistanceMetric as M
from desbordante.gdd import GddFromDot


class bcolors:
    HEADER = "\033[95m"
    WARNING = "\033[93m"
    ENDC = "\033[0m"


FIGURES_DIR = Path(r"examples/basic/verifying_gdd/figures")
IMAGE1_PATH = FIGURES_DIR / "capitals_fact_check1.png"
IMAGE2_PATH = FIGURES_DIR / "capitals_fact_check2.png"

PAPER_INFO = r"""This example demonstrates GDD validation for fact checking.

The pattern is defined in the paper

"Zhang, Y., Kwashie, S., Bewong, M., Hu, J., Mahboubi, A.,
Guo, X., & Feng, Z. Discovering graph differential dependencies.
Australasian Database Conference (ADC), 2023."
"""

GRAPH_INFO = r"""The figure contains a small fact-checking example.

On the left is the data graph. It stores cities and countries, together
with two kinds of facts: a city may be a capital of a country, and a
city may be located in a country.

Here we validate that a country's capital is located on that country's
territory. In graph terms, for one City vertex, the country reached via
`capital_of` must coincide with the country reached via `located_in`.

On the right is the graph pattern used by the dependency. Starting from
one City vertex, it follows both outgoing edges and binds the two
reached Country variables separately.
"""

GDD_INFO = r"""This example is intentionally built around a self-join pattern.

The pattern has one City variable and two Country variables.

It is important to distinguish graph isomorphism from graph homomorphism.

Informally, an isomorphism is a vertex bijection which is both edge-preserving
and label-preserving mapping. In particular, different pattern vertices must be
mapped to different graph vertices due to it's injectiveness.

A homomorphism is weaker: it must preserve the labeled edges of the
pattern, but it does not have to be injective. So two different pattern
vertices may be mapped to the same graph vertex, as long as all pattern
edges are still respected in the data graph.

Under the GDD definition from the paper, a match of the pattern is a
homomorphism, not an isomorphism. Therefore, variables 1 and 2 are not
required to map to different graph vertices.

So the two Country variables may refer to the same country node.
Here this is not a corner case but exactly the desired behavior: if
`capital_of` and `located_in` agree, both variables should be allowed
to map to the same Country.
"""

API_INFO = r"""This example constructs the dependency with GddFromDot, that is,
directly from a DOT string embedded in Python.

This is convenient for compact and self-contained examples. If the
pattern is already stored in the repository as a DOT file, the same
idea can be expressed with GddFromDotFile, as in the earlier
file-based validation example.

Now that the validator can also return counterexamples, we will use
that functionality on a second dataset where the fact-check fails.
"""

SHOWCASE_OK_INFO = r"""Showcase 1. Fact checking on a consistent dataset.

We validate the following rule on the pattern:

    if a city has both edges from the pattern,
    then the countries reached by `capital_of`
    and `located_in` must have the same name

This is exactly a fact-check that a country's capital is located on the
territory of that same country.

Formally, the right-hand side is

    1.name = 2.name

implemented as EDIT_DISTANCE == 0.0 between the two country names.

The left-hand side is empty. So the dependency is checked on every
homomorphic match of the pattern. This is a useful fact-checking
shape: two different graph paths that start from the same city must
agree on the country they reach.

In the current graph, Paris reaches France through both edges, and
Berlin reaches Germany through both edges. Lyon has no `capital_of`
edge, so it does not instantiate the pattern and does not participate
in the check. Therefore this dependency is expected to hold.
"""

SHOWCASE_BAD_INFO = r"""Showcase 2. Fact checking on an inconsistent dataset.

Now we keep the same schema, the same pattern, and the same GDD, but
we slightly modify the graph.

Paris is still recorded as the capital of France, but its `located_in`
edge now points to Germany.

So the dependency is expected not to hold. In this situation, the most
natural interpretation is that the data are inconsistent: two facts in
the graph disagree about the same city.

Since the validator can now return counterexamples, we will inspect
one violating match and see exactly how the pattern was mapped.
"""

COUNTEREXAMPLE_INFO = r"""A counterexample is a homomorphic match of the pattern for which
the implication fails.

Here the left-hand side is empty, so every match satisfies it. Thus a
counterexample is simply a match for which the right-hand side

    1.name = 2.name

does not hold.

Below we print how each pattern vertex was mapped to the graph.
"""

OUTRO_INFO = r"""In this example we used GDD validation as a consistency check
between two different paths in a graph.

Concretely, we checked that if a city is recorded as the capital of a
country, then that city is also recorded as being located in the same
country.

We also saw how a returned counterexample can be interpreted as a
concrete witness of a likely inconsistency in the data.
"""

GRAPH_DOT = r"""
digraph G {
    1 [label="City", name="Paris"];
    2 [label="City", name="Berlin"];
    3 [label="City", name="Lyon"];

    101 [label="Country", name="France"];
    102 [label="Country", name="Germany"];

    1 -> 101 [label="located_in"];
    2 -> 102 [label="located_in"];
    3 -> 101 [label="located_in"];

    1 -> 101 [label="capital_of"];
    2 -> 102 [label="capital_of"];
}
"""

INCONSISTENT_GRAPH_DOT = r"""
digraph G {
    1 [label="City", name="Paris"];
    2 [label="City", name="Berlin"];
    3 [label="City", name="Lyon"];

    101 [label="Country", name="France"];
    102 [label="Country", name="Germany"];

    1 -> 102 [label="located_in"];
    2 -> 102 [label="located_in"];
    3 -> 101 [label="located_in"];

    1 -> 101 [label="capital_of"];
    2 -> 102 [label="capital_of"];
}
"""

PATTERN_DOT = r"""
digraph P {
    0 [label="City"];
    1 [label="Country"];
    2 [label="Country"];

    0 -> 1 [label="capital_of"];
    0 -> 2 [label="located_in"];
}
"""

GDD_COUNTRY_AGREEMENT = GddFromDot(
    PATTERN_DOT,
    lhs=[],
    rhs=[
        AttrAttr(1, "name", 2, "name", M.EDIT_DISTANCE, Op.EQ, 0.0),
    ],
)


def write_temp_dot(name: str, dot: str) -> Path:
    tmpdir = Path(tempfile.mkdtemp(prefix=f"desbordante_{name}_"))
    path = tmpdir / f"{name}.dot"
    path.write_text(dot, encoding="utf-8")
    return path


def show_example(graph_image_path: Path) -> None:
    img = mpimg.imread(graph_image_path)
    plt.figure(figsize=(13, 6))
    plt.axis("off")
    plt.tight_layout(pad=0)
    plt.imshow(img)
    plt.show()


def validate_gdds(graph_dot: str, gdds: list) -> tuple[list, list]:
    graph_path = write_temp_dot("capitals_fact_check_graph", graph_dot)
    algo = desbordante.gdd.Default()
    algo.load_data(graph=str(graph_path), gdd=gdds)
    algo.execute()
    return algo.get_result(), algo.get_counterexamples()


def get_counterexample(counterexamples: list, gdd_index: int):
    return next(
        (ce for ce in counterexamples if ce.gdd_index == gdd_index),
        None,
    )


def print_counterexample(counterexample) -> None:
    print("Counterexample match:")
    for item in sorted(counterexample.match, key=lambda x: x.pattern_vertex_id):
        print(
            f"  {item.pattern_vertex_id} "
            f"{item.pattern_vertex_label} -> "
            f"{item.graph_vertex_id} "
            f"{item.graph_vertex_label} "
            f"{item.graph_vertex_attributes}"
        )
    print()


def print_block(title: str, text: str) -> None:
    print(f"{bcolors.HEADER}{title}{bcolors.ENDC}\n")
    print(text)
    print()


def main() -> None:
    print(PAPER_INFO)
    print_block(
        "Graph",
        GRAPH_INFO,
    )
    print_block(
        "Homomorphic matching",
        GDD_INFO,
    )
    print_block(
        "Python API used in this example",
        API_INFO,
    )

    print_block(
        "Showcase 1. Consistent dataset",
        SHOWCASE_OK_INFO,
    )

    valid_gdds, _ = validate_gdds(GRAPH_DOT, [GDD_COUNTRY_AGREEMENT])

    print(f"{bcolors.HEADER}Desbordante > {bcolors.ENDC}", end="")
    if GDD_COUNTRY_AGREEMENT in valid_gdds:
        print("GDD holds.\n")
    else:
        print("GDD does not hold.\n")

    print(f"{bcolors.WARNING}Close the image window to continue.{bcolors.ENDC}\n")
    show_example(IMAGE1_PATH)

    print_block(
        "Showcase 2. Inconsistent dataset",
        SHOWCASE_BAD_INFO,
    )

    valid_gdds, counterexamples = validate_gdds(
        INCONSISTENT_GRAPH_DOT,
        [GDD_COUNTRY_AGREEMENT],
    )

    print(f"{bcolors.HEADER}Desbordante > {bcolors.ENDC}", end="")
    if GDD_COUNTRY_AGREEMENT in valid_gdds:
        print("GDD holds.\n")
    else:
        print("GDD does not hold.\n")
        print(
            "This is likely a data inconsistency: one city is connected to "
            "different countries through `capital_of` and `located_in`.\n"
        )

        print_block(
            "Returned counterexample",
            COUNTEREXAMPLE_INFO,
        )

        counterexample = get_counterexample(counterexamples, 0)
        if counterexample is not None:
            print_counterexample(counterexample)
        else:
            print("No counterexample was returned.\n")

    print_block(
        "What we learned",
        OUTRO_INFO,
    )

    print(f"{bcolors.WARNING}Close the image window to finish.{bcolors.ENDC}")
    show_example(IMAGE2_PATH)


if __name__ == "__main__":
    main()
