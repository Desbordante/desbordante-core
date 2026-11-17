"""Discover exact constant CFDs with CFDMiner.

Run this example from the repository root:
    python3 examples/basic/mining_cfd_miner.py
"""

import csv
import textwrap
from pathlib import Path

import desbordante


DATASET_DIR = Path("examples/datasets/cfd_miner")
TIC_TAC_TOE_PATH = DATASET_DIR / "tic_tac_toe_endgames.csv"
ZOO_PATH = DATASET_DIR / "zoo.csv"


def print_paragraph(text):
    print(textwrap.fill(text, width=88))
    print()


def print_heading(title):
    print("=" * 88)
    print(title)
    print("=" * 88)


def read_csv(path):
    with path.open(newline="", encoding="utf-8") as file:
        return list(csv.DictReader(file))


def mine_cfds(path, minimum_support, maximum_lhs_size):
    algorithm = desbordante.cfd.algorithms.CFDMiner()
    algorithm.load_data(table=(str(path), ",", True))
    algorithm.execute(
        cfd_minsup=minimum_support,
        cfd_max_lhs=maximum_lhs_size,
    )
    return algorithm.get_cfds()


def to_named_rule(cfd, columns):
    lhs = tuple((columns[item.attribute], item.value) for item in cfd.lhs)
    rhs = (columns[cfd.rhs.attribute], cfd.rhs.value)
    return lhs, rhs


def rule_support(rule, rows):
    lhs, rhs = rule
    return sum(
        all(row[column] == value for column, value in lhs)
        and row[rhs[0]] == rhs[1]
        for row in rows
    )


def format_rule(rule):
    lhs, rhs = rule
    conditions = ", ".join(f"{column}={value}" for column, value in lhs)
    return f"({conditions}) -> ({rhs[0]}={rhs[1]})"


def select_rhs(cfds, columns, rhs_columns):
    return [
        to_named_rule(cfd, columns)
        for cfd in cfds
        if columns[cfd.rhs.attribute] in rhs_columns
    ]


def find_rule(rules, lhs, rhs):
    expected = (tuple(lhs), rhs)
    for rule in rules:
        if rule == expected:
            return rule
    raise RuntimeError(f"Could not find the expected CFD: {format_rule(expected)}")


def print_rules(rules, rows):
    for rule in rules:
        support = rule_support(rule, rows)
        print(f"  {format_rule(rule)} [support: {support}]")
    print()


def explain_algorithm():
    print_heading("Discovering exact constant CFDs with CFDMiner")
    print_paragraph(
        "A constant conditional functional dependency (constant CFD) "
        "has the form (X -> A, (t_p | a)). Here, X -> A is the underlying functional "
        "dependency, t_p is a set of constants for X, and a is a constant for A. Whenever a "
        "row matches t_p on the columns in X, its value in column A must be a. For "
        "example: ([Milk] -> Class, (yes | mammal)). This means that Milk=yes implies "
        "Class=mammal."
    )
    print_paragraph(
        "CFDMiner was developed by W. Fan, F. Geerts, J. Li, and M. Xiong. It first finds "
        "frequent free itemsets and their closures, then uses them to construct minimal "
        "constant CFDs. The algorithm discovers exact dependencies only: no row covered by "
        "the left-hand side of a discovered CFD may violate it."
    )
    print("Algorithm parameters:")
    print("  cfd_minsup   - minimum support expressed as a number of rows")
    print("  cfd_max_lhs  - maximum number of items on the left-hand side of a CFD")
    print()
    print_paragraph(
        "The support of a CFD is the number of table rows that satisfy it. A higher "
        "cfd_minsup requires a rule to be backed by more rows, so fewer rules remain in the "
        "result. Increasing cfd_max_lhs allows more complex combinations of attributes, but "
        "also expands the search space and usually increases the number of discovered rules."
    )


def describe_datasets(endgame_rows, zoo_rows):
    print_heading("Datasets")
    print_paragraph(
        f"Tic-Tac-Toe Endgame contains {len(endgame_rows)} records and "
        f"{len(endgame_rows[0])} columns. Each row describes a terminal position on a 3x3 "
        "board, while X_Wins indicates whether X won. Positions that can be transformed "
        "into one another by rotating or reflecting the board were merged, leaving 138 "
        "distinct configurations."
    )
    print_paragraph(
        f"Zoo contains {len(zoo_rows)} rows and {len(zoo_rows[0])} columns describing animal "
        "traits and one of seven biological classes."
    )


def scenario_winning_patterns(endgame_rows):
    print_heading("Scenario 1. Finding winning patterns")
    print_paragraph(
        "A winning line is defined by the values of three cells. We therefore allow up to "
        "three items on the left-hand side of a CFD and set the minimum support to 10. After "
        "running CFDMiner, we inspect the dependencies whose right-hand side is X_Wins=yes."
    )
    columns = list(endgame_rows[0])
    cfds = mine_cfds(TIC_TAC_TOE_PATH, minimum_support=10, maximum_lhs_size=3)
    outcome_rules = select_rhs(cfds, columns, {"X_Wins"})
    winning_rules = [
        find_rule(
            outcome_rules,
            [("Bottom_Left", "x"), ("Bottom_Middle", "x"), ("Bottom_Right", "x")],
            ("X_Wins", "yes"),
        ),
        find_rule(
            outcome_rules,
            [("Middle_Left", "x"), ("Center", "x"), ("Middle_Right", "x")],
            ("X_Wins", "yes"),
        ),
        find_rule(
            outcome_rules,
            [("Top_Right", "x"), ("Middle_Right", "x"), ("Bottom_Right", "x")],
            ("X_Wins", "yes"),
        ),
        find_rule(
            outcome_rules,
            [("Top_Right", "x"), ("Center", "x"), ("Bottom_Left", "x")],
            ("X_Wins", "yes"),
        ),
    ]
    print(f"CFDMiner found {len(cfds)} CFDs in total.")
    print(f"The right-hand side determines the game outcome in {len(outcome_rules)} of them.")
    print("Here are four frequent rules corresponding to a win by X:")
    print_rules(winning_rules, endgame_rows)
    print_paragraph(
        "In this sense, the algorithm reconstructs the underlying logic of the game "
        "directly from the dataset."
    )


def scenario_lhs_limit(endgame_rows):
    print_heading("Scenario 2. Choosing the left-hand-side size limit")
    print_paragraph(
        "The cfd_max_lhs parameter affects more than execution time: it determines which "
        "patterns the algorithm can express at all. A winning pattern occupies three cells, "
        "so a two-item limit cannot describe it completely."
    )
    columns = list(endgame_rows[0])
    for maximum_lhs_size in (2, 3):
        cfds = mine_cfds(TIC_TAC_TOE_PATH, minimum_support=10, maximum_lhs_size=maximum_lhs_size)
        outcome_rules = select_rhs(cfds, columns, {"X_Wins"})
        print(
            f"  cfd_max_lhs={maximum_lhs_size}: {len(cfds)} CFDs in total, "
            f"CFDs for X_Wins: {len(outcome_rules)}"
        )
    print()
    print_paragraph(
        "With cfd_max_lhs=2, the algorithm finds no exact rule for X_Wins with support of "
        "at least 10."
    )


def scenario_interpreting_cfds(zoo_rows):
    print_heading("Scenario 3. Examining biological patterns")
    print_paragraph(
        "For Zoo, we limit the left-hand side to one item and set the minimum support to 20. "
        "To make the resulting dependencies easier to interpret, we examine several of the "
        "most illustrative ones in groups."
    )
    columns = list(zoo_rows[0])
    cfds = mine_cfds(ZOO_PATH, minimum_support=20, maximum_lhs_size=1)
    rules = [to_named_rule(cfd, columns) for cfd in cfds]

    mammal_rules = [
        find_rule(rules, [("Milk", "yes")], ("Class", "mammal")),
        find_rule(rules, [("Class", "mammal")], ("Milk", "yes")),
    ]
    bird_rules = [
        find_rule(rules, [("Feathers", "yes")], ("Class", "bird")),
        find_rule(rules, [("Class", "bird")], ("Feathers", "yes")),
        find_rule(rules, [("Feathers", "yes")], ("Eggs", "yes")),
        find_rule(rules, [("Feathers", "yes")], ("Legs", "2")),
    ]
    cross_class_rules = [
        find_rule(rules, [("Toothed", "yes")], ("Backbone", "yes")),
        find_rule(rules, [("Airborne", "yes")], ("Breathes", "yes")),
    ]

    print(f"CFDMiner found {len(cfds)} CFDs in total.")
    print("A pair of converse rules for mammals:")
    print_rules(mammal_rules, zoo_rows)
    print_paragraph(
        "Each rule has support 41. In other words, the rows with Milk=yes are exactly the "
        "rows with Class=mammal in the Zoo dataset."
    )

    print("Now consider the dependencies that describe birds:")
    print_rules(bird_rules, zoo_rows)
    print_paragraph(
        "In Zoo, every animal with Feathers=yes belongs to the bird class, and every bird "
        "has feathers: both dependencies describe the same group of 20 rows. In addition, "
        "every animal in this group has Eggs=yes and Legs=2. The discovered rules therefore "
        "describe several shared traits at once."
    )

    print("Patterns not tied to a single class:")
    print_rules(cross_class_rules, zoo_rows)
    print_paragraph(
        "This example shows that a useful CFD for this dataset does not have to include the "
        "Class column. Toothed animals belong to different classes, yet Backbone=yes in all "
        "61 corresponding rows. Similarly, Breathes=yes in all 24 rows with Airborne=yes. "
        "The converse dependencies do not hold for every row."
    )


def print_summary():
    print_heading("Summary")
    print_paragraph(
        "CFDMiner is designed to find exact dependencies tied to specific values. The "
        "cfd_minsup parameter sets the required amount of supporting data, while cfd_max_lhs "
        "limits the complexity of the context. The discovered dependencies are easier to "
        "understand when analyzed in groups and interpreted using domain knowledge. For CFDs "
        "with wildcards or rules that allow violations, use the other algorithms shown in "
        "examples/basic/mining_cfd.py."
    )


def main():
    endgame_rows = read_csv(TIC_TAC_TOE_PATH)
    zoo_rows = read_csv(ZOO_PATH)

    explain_algorithm()
    describe_datasets(endgame_rows, zoo_rows)
    scenario_winning_patterns(endgame_rows)
    scenario_lhs_limit(endgame_rows)
    scenario_interpreting_cfds(zoo_rows)
    print_summary()


if __name__ == "__main__":
    main()
