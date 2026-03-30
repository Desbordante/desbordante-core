"""
This example shows how GDD validation can be used for fact checking.

Intuition:
- if a city has an outgoing `capital_of` edge to some country,
- and the same city has an outgoing `located_in` edge to some country,
- then these two countries must be the same.

This is a non-trivial dependency:
the rule does not repeat the same fact, but compares two different graph paths
that should agree semantically.
"""

from __future__ import annotations

import desbordante as d
from desbordante.gdd import (
    GddFromDot,
    AttrConst,
    AttrAttr,
    DistanceMetric as M,
    CmpOp as Op,
)

from pathlib import Path
import tempfile


def write_temp_dot(name: str, dot: str) -> Path:
    tmpdir = Path(tempfile.mkdtemp(prefix=f"desbordante_{name}_"))
    path = tmpdir / f"{name}.dot"
    path.write_text(dot, encoding="utf-8")
    return path


def validate_gdds(graph_dot: str, gdds: list, graph_name: str = "graph"):
    graph_path = write_temp_dot(graph_name, graph_dot)
    algo = d.gdd.Default()
    algo.load_data(graph=str(graph_path), gdd=gdds)
    algo.execute()
    return algo.get_result()


def assert_contains_gdd(valid_gdds: list, target):
    assert target in valid_gdds, (
        f"GDD not found in result.\n"
        f"Target: {target}\n"
        f"Valid: {valid_gdds}"
    )


def assert_not_contains_gdd(valid_gdds: list, target):
    assert target not in valid_gdds, (
        f"GDD unexpectedly found in result.\n"
        f"Target: {target}\n"
        f"Valid: {valid_gdds}"
    )


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

def validate_gdds(graph_dot: str, gdds: list):
    import tempfile
    from pathlib import Path

    tmpdir = Path(tempfile.mkdtemp(prefix="desbordante_gdd_dblp_"))
    graph_path = tmpdir / "graph.dot"
    graph_path.write_text(graph_dot, encoding="utf-8")

    algo = d.gdd.Default()
    algo.load_data(graph=str(graph_path), gdd=gdds)
    algo.execute()
    return algo.get_result()


def contains_gdd(valid_gdds, target):
    return target in valid_gdds


def main():
    # GDD:
    #
    #   if a match of the pattern exists,
    #   then the country reached via `capital_of`
    #   must be the same as the country reached via `located_in`.
    #
    #   country_1.name == country_2.name
    fact_check_capital_gdd = GddFromDot(
        PATTERN_DOT,
        lhs=[],
        rhs=[
            AttrAttr(1, "name", 2, "name", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    gdds = [fact_check_capital_gdd]
    valid_gdds = validate_gdds(GRAPH_DOT, gdds)

    print("Input GDDs:")
    for i, gdd in enumerate(gdds, start=1):
        print(f"  {i}. {gdd}")

    print("\nValid GDDs:")
    for i, gdd in enumerate(valid_gdds, start=1):
        print(f"  {i}. {gdd}")

    assert len(valid_gdds) == 1, f"Expected exactly one valid GDD, got {len(valid_gdds)}"
    assert contains_gdd(valid_gdds, fact_check_capital_gdd)

    print("\nInterpretation:")
    print("- Our hypothesis has been proved via verification by gdd.")
    print("- This illustrates how GDD validation can be used for fact checking:")
    print("  it finds graph facts that are inconsistent with other graph facts.")


if __name__ == "__main__":
    main()
