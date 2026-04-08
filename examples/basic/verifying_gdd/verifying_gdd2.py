from pathlib import Path
import tempfile

import matplotlib.image as mpimg
import matplotlib.pyplot as plt

import desbordante
from desbordante.gdd import AttrAttr, CmpOp as Op, DistanceMetric as M, GddFromDot


class bcolors:
    HEADER = "\033[95m"
    WARNING = "\033[93m"
    ENDC = "\033[0m"


FIGURES_DIR = Path(r"examples/basic/verifying_gdd/figures")
IMAGE_PATH = FIGURES_DIR / "capitals_fact_check.png"

PAPER_INFO = r"""This example demonstrates GDD validation for fact checking.

The primitive is defined in the paper

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

The pattern has one City variable and two Country variables. Under the
GDD definition from the paper, a match of the pattern is a
homomorphism, not an isomorphism. Therefore, variables 1 and 2 are not
required to map to different graph vertices.

So the two Country variables may be glued to the same country node.
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
"""

SHOWCASE_INFO = r"""Showcase. Fact checking via path agreement.

We validate the following rule on the pattern:

    if a city has both edges from the pattern,
    then the countries reached by `capital_of`
    and `located_in` must have the same name

This is exactly a fact-check that a country's capital is located on the
territory of that same country.

Formally, the right-hand side is

    1.name = 2.name

implemented as EDIT_DISTANCE <= 0.0 between the two country names.

The left-hand side is empty. So the dependency is checked on every
homomorphic match of the pattern. This is a useful fact-checking
shape: two different graph paths that start from the same city must
agree on the country they reach.

In the current graph, Paris reaches France through both edges, and
Berlin reaches Germany through both edges. Lyon has no `capital_of`
edge, so it does not instantiate the pattern and does not participate
in the check. Therefore this dependency is expected to hold.
"""

OUTRO_INFO = r"""In this example we used GDD validation as a consistency check
between two different paths in a graph.

Concretely, we checked that if a city is recorded as the capital of a
country, then that city is also recorded as being located in the same
country.
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
        AttrAttr(1, "name", 2, "name", M.EDIT_DISTANCE, Op.LE, 0.0),
    ],
)

SHOWCASES = [
    ("Showcase", SHOWCASE_INFO, GDD_COUNTRY_AGREEMENT),
]


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


def validate_gdds(graph_dot: str, gdds: list) -> list:
    graph_path = write_temp_dot("capitals_fact_check_graph", graph_dot)
    algo = desbordante.gdd.Default()
    algo.load_data(graph=str(graph_path), gdd=gdds)
    algo.execute()
    return algo.get_result()


def main() -> None:
    print(PAPER_INFO)
    print(GRAPH_INFO)
    print(GDD_INFO)
    print(API_INFO)

    gdds = [gdd for _, _, gdd in SHOWCASES]
    valid_gdds = validate_gdds(GRAPH_DOT, gdds)

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
