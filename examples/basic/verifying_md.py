import desbordante
import pandas as pd

from typing import TypedDict
from desbordante.md_verification import ColumnSimilarityClassifier
from desbordante.md_verification.similarity_measure import (
    EuclideanSimilarity,
    LevenshteinSimilarity,
)

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"


class MDParams(TypedDict):
    lhs: list[ColumnSimilarityClassifier]
    rhs: ColumnSimilarityClassifier


def print_results(verifier):
    if verifier.md_holds():
        print(GREEN_CODE, "MD holds", DEFAULT_COLOR_CODE, "\n")
        return

    highlights = verifier.get_highlights()
    print(RED_CODE, "MD does not hold due to the following items:", DEFAULT_COLOR_CODE)
    for highlight in highlights:
        print(highlight.ToString())
    print(
        f"Desbordante suggests to use following right-hand side decision boundary: {verifier.get_rhs_suggestions()}\n"
    )


def check_md(table_path: str, params: MDParams):
    algo = desbordante.md_verification.algorithms.Default()
    algo.load_data(left_table=(table_path, ",", True))

    algo.execute(**params)
    print_results(algo)


def animals_beverages_example():
    print("As first example, let's look at the dataset animals_beverages.csv")

    table_path = "examples/datasets/animals_beverages.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nLet's try to check if MD {Levenshtein(animal, animal) -> Levenshtein(diet, diet)} with decision boundaries {1.0} and {1.0} respectively.\n"
        "Levenshtein similarity with decision boundary equal to 1.0 means that values must be equal.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(2, 2, LevenshteinSimilarity(), 1)],
        "rhs": ColumnSimilarityClassifier(3, 3, LevenshteinSimilarity(), 1),
    }

    check_md(table_path, params)

    print(
        "As we see, such MD doesn't holds due to some sort of typo in table.\n"
        "Let's ease our constraints and say that if column's similarity measure is at least 0.75, they are similar enough:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(2, 2, LevenshteinSimilarity(), 0.75)],
        "rhs": ColumnSimilarityClassifier(3, 3, LevenshteinSimilarity(), 0.75),
    }

    check_md(table_path, params)

    print(
        "As a result, MD {[Levenshtein(animal, animal) >= 0.75] -> [Levenshtein(diet, diet) >= 0.75]} holds.\n"
        "This is how MD can be helpful in avoiding typos in table and searching them\n\n"
    )

    print(
        "Now let's take a look that happen if we increase decision boundary of left-hand side and right-hand side\n"
        "For example, we will take 0.76 instead of 0.75"
    )
    print("Left-hand side:\n")

    params = {
        "lhs": [ColumnSimilarityClassifier(2, 2, LevenshteinSimilarity(), 0.76)],
        "rhs": ColumnSimilarityClassifier(3, 3, LevenshteinSimilarity(), 0.75),
    }

    check_md(table_path, params)

    print(
        "As we can see, nothing changed. Now let's increase right-hand side decision boundary:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(2, 2, LevenshteinSimilarity(), 0.75)],
        "rhs": ColumnSimilarityClassifier(3, 3, LevenshteinSimilarity(), 0.76),
    }

    check_md(table_path, params)

    print(
        'Values "meat" and "mead" have Levenshtein similarity measure equal to 0.75, but we accept similarity measure at least 0.76, so MD doesn\'t holds.\n'
    )


def theatre_example():
    print(
        "Let's look at the example with numeric columns.\nWe will use theatre.csv dataset for such purpose:\n"
    )

    table_path = "examples/datasets/theatres_typos.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nAs we see, there are some typos in this dataset.\n"
        "We will try to discover MD {Levenshtein(Title, Title) -> Levenshtein(Duration, Duration)}\nFirstly, let's check if MD with all decision boundaries equal to 1.0 holds:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(0, 0, LevenshteinSimilarity(), 1)],
        "rhs": ColumnSimilarityClassifier(2, 2, EuclideanSimilarity(), 1),
    }

    check_md(table_path, params)

    print("To avoid typos, let's set left-hand side decision boundary to 0.75:\n")

    params = {
        "lhs": [ColumnSimilarityClassifier(0, 0, LevenshteinSimilarity(), 0.75)],
        "rhs": ColumnSimilarityClassifier(2, 2, EuclideanSimilarity(), 1),
    }

    check_md(table_path, params)

    print("More pairs violationg MD appeared.\n")

    print(
        "Desbordante suggest to use right-hand side decision boundary lower than 0.1(6).\n"
        "Let's try to verify MD {[Levenshtein(Title, Title) >= 0.75] -> [Levenshtein(Duration, Duration) >= 0.165]}\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(0, 0, LevenshteinSimilarity(), 0.75)],
        "rhs": ColumnSimilarityClassifier(2, 2, EuclideanSimilarity(), 0.165),
    }

    check_md(table_path, params)

    print("As we see, now everything holds.")


if __name__ == "__main__":
    print(DEFAULT_COLOR_CODE)
    print(
        "This example demonstrates how to validate Matching Dependancies (MD) using the Desbordante library. "
        "You can read about Matching Dependancies and their formal definition in article below\n"
        "https://hpi.de/fileadmin/user_upload/fachgebiete/naumann/publications/PDFs/2020_schirmer_efficient.pdf\n"
    )
    print(
        "To verify Matching Dependancy, firstly we must define Column Similarity Classifiers for tables.\n"
        "Column Similarity Classifier consists of Column Match and decision boundary. "
        "Column Match consists of two indices: columns in left and right table and similarity measure (Levevnshtein Similarity, for example).\n"
        "We will use notation [measure(i, j) >= lambda] for Column Similarity Classifier with i'th column of left table, j'th column of right table, similarity measure 'measure' and decision boundary lambda."
        "To clarify, in this example we'll write column names instead of column indices.\n"
    )
    animals_beverages_example()
    print("-" * 50)
    theatre_example()
