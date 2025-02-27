import desbordante
import pandas as pd

from typing import TypedDict
from desbordante.md_verifier.similarity_measures import (
    SimilarityMeasure,
    EuclideanSimilarity,
    LevenshteinSimilarity,
)

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"


class MDParams(TypedDict):
    lhs_indices: list[int]
    rhs_indices: list[int]
    lhs_decision_boundaries: list[float]
    rhs_decision_boundaries: list[float]
    lhs_similarity_measures: list[SimilarityMeasure]
    rhs_similarity_measures: list[SimilarityMeasure]


def print_results(verifier):
    if verifier.md_holds():
        print(GREEN_CODE, "MD holds", DEFAULT_COLOR_CODE, "\n")
        return

    highlights = verifier.get_highlights()
    print(RED_CODE, "MFD does not hold due to the following items:", DEFAULT_COLOR_CODE)
    for highlight in highlights:
        print(
            f"Rows {highlight.rows[0]} and {highlight.rows[1]} violate MD in column {highlight.column}: "
            f'"{highlight.first_value}" and "{highlight.second_value}" have similarity {highlight.similarity} '
            f"with decision boundary {highlight.decision_boundary}\n"
        )
    print(
        f"Desbordante suggests to use following right-hand side decision boundaries: {verifier.get_rhs_suggestions()}\n"
    )


def check_md(table_path: str, params: MDParams):
    algo = desbordante.md_verifier.algorithms.Default()
    algo.load_data(table=(table_path, ",", True))

    algo.execute(**params)
    print_results(algo)


def drunk_animals():
    print("As first example, let's look at the dataset drunk_animals.csv")

    table_path = "test_input_data/drunk_animals.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nLet's try to check if MD {animal -> diet} with decision boundaries {1.0} and {1.0} and Levenshtein similarity measure holds.\n"
        "Matching Dependancy with all decision boundaries equal to 1.0 is the same as Functional Dependancy.\n"
    )

    params = {
        "lhs_indices": [2],
        "rhs_indices": [3],
        "lhs_decision_boundaries": [1],
        "rhs_decision_boundaries": [1],
        "lhs_similarity_measures": [LevenshteinSimilarity()],
        "rhs_similarity_measures": [LevenshteinSimilarity()],
    }

    check_md(table_path, params)

    print(
        "As we see, Functional Dependancy doesn't holds due to some sort of typo in table.\n"
        "Let's ease our constraints and say that if column's similarity measure is at least 0.75, they are similar enough to be equal:\n"
    )

    params = {
        "lhs_indices": [2],
        "rhs_indices": [3],
        "lhs_decision_boundaries": [0.75],
        "rhs_decision_boundaries": [0.75],
        "lhs_similarity_measures": [LevenshteinSimilarity()],
        "rhs_similarity_measures": [LevenshteinSimilarity()],
    }

    check_md(table_path, params)

    print(
        "As a result, MD holds.\n"
        "This is how MD can be helpful in avoiding typos in table and searching them\n\n"
    )


if __name__ == "__main__":
    print(DEFAULT_COLOR_CODE)
    print(
        "This example demonstrates how to validate Matching Dependancies (MD) using the Desbordante library. "
        "Let's assume we have left-hand side columns X1, X2 ..., right-hand side columns Y1, Y2, ..., "
        "left-hand side decision boundaries Xb1, Xb2 ..., right-hand side decision boundaries Xb1, Xb2 ... "
        "and some similarity measures SimX1, SimX2, SimY1, SimY2, ... for each of columns above. "
        "Matching Dependancy {X1, X2, ... -> Y1, Y2, ...} holds if for every row i and j from table "
        "if SimX1(X1_i, X1_j) < Xb1 && SimX2(X2_i, X2_j) < Xb2 && ... is true then "
        "SimY1(Y1_i, Y1_j) < Yb1 && SimY2(Y2_i, Y2_j) < Yb2 && ... is also true "
        "You can read more about Matching Dependancies in article below\n"
        "https://arxiv.org/pdf/0903.3317\n"
    )
    drunk_animals()
