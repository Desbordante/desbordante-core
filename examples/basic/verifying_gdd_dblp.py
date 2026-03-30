"""
DBLP-like GDD validation example.

This example is inspired by the official DBLP bibliographic dataset:
authors, papers, venues, years.

The graph below is synthetic, but it uses a realistic bibliographic schema:
Author -> Paper -> Venue, with author disambiguation via canonical_author_id.

The example demonstrates two GDDs:
1. a stronger author-resolution rule that SHOULD hold;
2. a weaker rule that SHOULD fail.
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
    1 [label="Author", name="Jiawei Han", canonical_author_id="author:han_jiawei"];
    2 [label="Author", name="J. Han",     canonical_author_id="author:han_jiawei"];

    3 [label="Author", name="Philip S. Yu", canonical_author_id="author:yu_philip"];

    4 [label="Author", name="Yi Zhang", canonical_author_id="author:zhang_yi"];
    5 [label="Author", name="Yu Zhang", canonical_author_id="author:zhang_yu"];

    101 [label="Paper", title="Mining Frequent Patterns",     year="2000"];
    102 [label="Paper", title="Mining Frequent Pattern Sets", year="2000"];
    103 [label="Paper", title="Scalable Pattern Search",      year="2023"];
    104 [label="Paper", title="Efficient Pattern Search",     year="2023"];

    201 [label="Venue", name="SIGMOD"];
    202 [label="Venue", name="KDD"];

    1 -> 101 [label="authored"];
    3 -> 101 [label="authored"];

    2 -> 102 [label="authored"];
    3 -> 102 [label="authored"];

    4 -> 103 [label="authored"];
    5 -> 104 [label="authored"];

    101 -> 201 [label="published_in"];
    102 -> 201 [label="published_in"];
    103 -> 202 [label="published_in"];
    104 -> 202 [label="published_in"];
}
"""

# Strong pattern:
# two authors, two papers, one common coauthor, one common venue
STRONG_PATTERN = r"""
digraph P {
    0 [label="Author"];
    1 [label="Author"];
    2 [label="Paper"];
    3 [label="Paper"];
    4 [label="Author"];
    5 [label="Venue"];

    0 -> 2 [label="authored"];
    1 -> 3 [label="authored"];
    4 -> 2 [label="authored"];
    4 -> 3 [label="authored"];
    2 -> 5 [label="published_in"];
    3 -> 5 [label="published_in"];
}
"""

# Weak pattern:
# two authors, two papers, same venue only
WEAK_PATTERN = r"""
digraph P {
    0 [label="Author"];
    1 [label="Author"];
    2 [label="Paper"];
    3 [label="Paper"];
    4 [label="Venue"];

    0 -> 2 [label="authored"];
    1 -> 3 [label="authored"];
    2 -> 4 [label="published_in"];
    3 -> 4 [label="published_in"];
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
    # Strong rule:
    # close names + same venue + same year + shared coauthor => same canonical author id
    strong_gdd = GddFromDot(
        STRONG_PATTERN,
        lhs=[
            AttrAttr(0, "name", 1, "name", M.EDIT_DISTANCE, Op.LE, 8.0),
            AttrAttr(2, "year", 3, "year", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
        rhs=[
            AttrAttr(0, "canonical_author_id", 1, "canonical_author_id", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    # Weak rule:
    # close names + same venue + same year => same canonical author id
    weak_gdd = GddFromDot(
        WEAK_PATTERN,
        lhs=[
            AttrAttr(0, "name", 1, "name", M.EDIT_DISTANCE, Op.LE, 2.0),
            AttrAttr(2, "year", 3, "year", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
        rhs=[
            AttrAttr(0, "canonical_author_id", 1, "canonical_author_id", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    gdds = [weak_gdd, strong_gdd]
    valid_gdds = validate_gdds(GRAPH_DOT, gdds)

    print("Input GDDs:")
    for i, gdd in enumerate(gdds, start=1):
        print(f"  {i}. {gdd}")

    print("\nValid GDDs:")
    for i, gdd in enumerate(valid_gdds, start=1):
        print(f"  {i}. {gdd}")

    assert len(valid_gdds) == 1, f"Expected exactly one valid GDD, got {len(valid_gdds)}"
    assert contains_gdd(valid_gdds, strong_gdd), "The strong GDD should hold on this graph."
    assert not contains_gdd(valid_gdds, weak_gdd), "The weak GDD should fail on this graph."

    print("\nInterpretation:")
    print("- strong_gdd holds: 'Jiawei Han' and 'J. Han' share venue, year, and coauthor Philip S. Yu.")
    print("- weak_gdd fails: 'Yi Zhang' and 'Yu Zhang' are close in name and publish in KDD 2023,")
    print("  but they are distinct authors, so the implication is false.")


if __name__ == "__main__":
    main()
