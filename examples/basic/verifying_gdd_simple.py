"""
Basic example: validating graph differential dependencies (GDDs).

A GDD has the form:

    (pattern, lhs -> rhs)

Interpretation:
for every homomorphic match of `pattern` in the graph,
if all constraints from `lhs` hold on that match,
then all constraints from `rhs` must also hold.

This example shows:
1. how to describe a graph and a pattern in DOT;
2. how to build GDDs from attribute and relation constraints;
3. how to run the validator;
4. how to understand the result.

Current Python bindings expose:
- desbordante.gdd.Gdd
- desbordante.gdd.GddFromDot / GddFromDotFile
- desbordante.gdd.GddToken, AttrTag, RelTag
- desbordante.gdd.DistanceConstraint
- helpers: AttrConst, AttrAttr, RelConst, RelRel
- desbordante.gdd.DistanceMetric: ABS_DIFF, EDIT_DISTANCE
- desbordante.gdd.CmpOp: EQ, LE
"""

from pathlib import Path
import tempfile

import desbordante as d
from desbordante.gdd import (
    GddFromDot,
    AttrConst,
    AttrAttr,
    RelConst,
    DistanceMetric as M,
    CmpOp as Op,
)


GRAPH_DOT = r"""
digraph G {
    1 [label="Person", name="Misha", age="25"];
    2 [label="Person", name="Misha", age="31"];
    3 [label="Person", name="Bob",   age="25"];

    101 [label="City", name="Amsterdam"];
    102 [label="City", name="Riga"];

    1 -> 101 [label="lives_in"];
    2 -> 102 [label="lives_in"];
    3 -> 101 [label="lives_in"];
}
"""

# Pattern variables are identified by vertex ids inside the DOT pattern.
# Here:
#   0 -> Person
#   1 -> City
#
# The validator will enumerate all homomorphic matches of this pattern
# in the graph and check lhs -> rhs on every such match.
PATTERN_DOT = r"""
digraph P {
    0 [label="Person"];
    1 [label="City"];
    0 -> 1 [label="lives_in"];
}
"""


def save_temp_graph(dot: str) -> Path:
    tmpdir = Path(tempfile.mkdtemp(prefix="desbordante_gdd_intro_"))
    path = tmpdir / "graph.dot"
    path.write_text(dot, encoding="utf-8")
    return path


def validate(graph_path: Path, gdds: list):
    algo = d.gdd.Default()
    algo.load_data(graph=str(graph_path), gdd=gdds)
    algo.execute()
    return algo.get_result()


def main():
    graph_path = save_temp_graph(GRAPH_DOT)

    # Rule 1:
    # "If the matched person's name is exactly 'Misha',
    #  then the matched city must be 'Amsterdam'."
    #
    # lhs uses an attribute-to-constant constraint.
    # rhs uses another attribute-to-constant constraint.
    #
    # In our graph this rule is FALSE, because vertex 2 is also named Misha
    # but lives in Riga.
    gdd_misha_lives_in_amsterdam = GddFromDot(
        PATTERN_DOT,
        lhs=[
            AttrConst(0, "name", "Misha", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
        rhs=[
            AttrConst(1, "name", "Amsterdam", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    # Rule 2:
    # "If the matched person's age is 25,
    #  then the relation `lives_in` must point to node 101."
    #
    # This demonstrates a relation constraint on the RHS.
    #
    # In our graph this rule is TRUE:
    # - person 1 has age 25 and lives in 101
    # - person 3 has age 25 and lives in 101
    gdd_age_25_implies_city_101 = GddFromDot(
        PATTERN_DOT,
        lhs=[
            AttrConst(0, "age", 25, M.ABS_DIFF, Op.LE, 0.0),
        ],
        rhs=[
            RelConst(0, "lives_in", 101),
        ],
    )

    # Rule 3:
    # "If the person's name is close to 'Masha' with edit distance <= 1,
    #  then the city's name is close to 'Amsterdam' with edit distance <= 2."
    #
    # Also FALSE on this graph because one of the Misha matches leads to Riga.
    gdd_soft_name_rule = GddFromDot(
        PATTERN_DOT,
        lhs=[
            AttrConst(0, "name", "Masha", M.EDIT_DISTANCE, Op.LE, 1.0),
        ],
        rhs=[
            AttrConst(1, "name", "Amsterdam", M.EDIT_DISTANCE, Op.LE, 2.0),
        ],
    )

    # Rule 4:
    # "If the person's name is close to 'Masha' with edit distance <= 1,
    #  and the person's age is less then 30,
    #  then the city's name is close is exactly 'Riga'
    #
    # Also FALSE on this graph because one of the Misha matches leads to Riga.
    gdd_name_and_age_rule = GddFromDot(
        PATTERN_DOT,
        lhs=[
            AttrConst(0, "name", "Masha", M.EDIT_DISTANCE, Op.LE, 1.0),
            AttrConst(0, "age", 29, M.ABS_DIFF, Op.EQ, 0.0),
        ],
        rhs=[
            AttrConst(1, "name", "Amsterdam", M.EDIT_DISTANCE, Op.LE, 2.0),
        ],
    )

    gdds = [
        gdd_misha_lives_in_amsterdam,
        gdd_age_25_implies_city_101,
        gdd_soft_name_rule,
        gdd_name_and_age_rule
    ]

    valid_gdds = validate(graph_path, gdds)

    print("Input GDDs:")
    for i, gdd in enumerate(gdds, start=1):
        print(f"  {i}. {gdd}")

    print("\nValid GDDs returned by the validator:")

    for i, gdd in enumerate(valid_gdds, start=1):
        print(f"  {i}. {gdd}")

    print("\nExpected outcome:")
    print("- 'Misha -> Amsterdam' is invalid because one Misha lives in Riga.")
    print("- 'age = 25 -> lives_in(101)' is valid.")
    print("- the soft name rule is invalid for the same reason as the first rule.")
    print("- 'age <= 29 and name ~ Masha -> lives_in(101)' is valid.")

    assert len(valid_gdds) == 2
    assert gdd_age_25_implies_city_101 in valid_gdds
    assert gdd_name_and_age_rule in valid_gdds


if __name__ == "__main__":
    main()
