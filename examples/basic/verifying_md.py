import desbordante
import pandas as pd

from typing import TypedDict
from desbordante.md import ColumnSimilarityClassifier
from desbordante.md.column_matches import Levenshtein, Custom

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
        print(highlight.to_string())
    print(
        f"Desbordante suggests to use following right-hand side decision boundary: {verifier.get_true_rhs_decision_boundary()}\n"
    )
    print(f"Following MD was provided:\n  {verifier.get_input_md()}")
    print("Following MDs are suggested:")
    for md in verifier.get_md_suggestions():
        print(f"  {md.to_string_active()}")
    print()


def check_md(table: str, params: MDParams):
    algo = desbordante.md_verification.algorithms.Default()
    algo.load_data(left_table=table)

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
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1),
    }

    check_md(table, params)

    print(
        "As we see, such MD doesn't holds due to some sort of typo in table.\n"
        "Let's ease our constraints and say that if column's similarity measure is at least 0.75, they are similar enough:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

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
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.76)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

    print(
        "As we can see, nothing changed. Now let's increase right-hand side decision boundary:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.76),
    }

    check_md(table, params)

    print(
        'Values "meat" and "mead" have Levenshtein similarity measure equal to 0.75, but we accept similarity measure at least 0.76, so MD doesn\'t holds.\n'
    )

    print("Let's check that changes if we corrent typos in dataset")

    table["animal"] = table["animal"].replace({"beer": "bear"})
    table["diet"] = table["diet"].replace({"mead": "meat"})

    print(f"Corrected dataset:\n\n{table}\n")
    print("Now let's check MD with 1.0 decision boundaries again")

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1.0),
    }

    check_md(table, params)


def theatre_example():
    print(
        "Let's look at the example with numeric columns.\nWe will use theatre.csv dataset for such purpose:\n"
    )

    table_path = "examples/datasets/theatres_typos.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nAs we see, there are some typos in this dataset.\n"
        "We will try to discover MD {Levenshtein(Title, Title) -> NormalizedDistance(Duration, Duration)}\nFirstly, let's check if MD with all decision boundaries equal to 1.0 holds:\n"
    )

    max_duration = max(table["Duration"])

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("Title", "Title", 0.0), 1)],
        "rhs": ColumnSimilarityClassifier(
            Custom(
                lambda d1, d2: 1 - abs(int(d1) - int(d2)) / max_duration,
                "Duration",
                "Duration",
                symmetrical=True,
                equality_is_max=True,
                measure_name="normalized_distance",
                min_sim=0.0,
            ),
            1,
        ),
    }

    check_md(table, params)

    print("To avoid typos, let's set left-hand side decision boundary to 0.75:\n")

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("Title", "Title", 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(
            Custom(
                lambda d1, d2: 1 - abs(int(d1) - int(d2)) / max_duration,
                "Duration",
                "Duration",
                symmetrical=True,
                equality_is_max=True,
                measure_name="normalized_distance",
                min_sim=0.0,
            ),
            1,
        ),
    }

    check_md(table, params)

    print("More pairs violationg MD appeared.\n")

    print(
        "Desbordante suggest to use right-hand side decision boundary lower than 0.(96).\n"
        "Let's try to verify MD {[Levenshtein(Title, Title) >= 0.75] -> [Levenshtein(Duration, Duration) >= 0.96]}\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("Title", "Title", 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(
            Custom(
                lambda d1, d2: 1 - abs(int(d1) - int(d2)) / max_duration,
                "Duration",
                "Duration",
                symmetrical=True,
                equality_is_max=True,
                measure_name="normalized_distance",
                min_sim=0.0,
            ),
            0.96,
        ),
    }

    check_md(table, params)

    print("As we see, now everything holds.")


if __name__ == "__main__":
    print(DEFAULT_COLOR_CODE)
    print(
        "This example demonstrates how to validate Matching Dependancies (MD) from 'Efficient Discovery of Matching Dependencies' "
        "by Schirmer et al. using the Desbordante library. "
        "You can read about Matching Dependancies and their formal definition in article below\n"
        "https://hpi.de/fileadmin/user_upload/fachgebiete/naumann/publications/PDFs/2020_schirmer_efficient.pdf\n"
    )
    print(
        "Matching dependancies verification algorithm accepts left-hand side and right-hand side and returns if such dependancy holds. "
        "Also in case if dependancy doesn't hold, algorithm also returns list of highlights and suggests how to adjust dependancy."
    )
    print(
        "You can also read about mining Matching Dependancies in examples/basic/mining_md.py"
    )

    print(
        "To verify Matching Dependancy, firstly we must define Column Similarity Classifiers for tables.\n"
        "Column Similarity Classifier consists of Column Match and decision boundary. "
        "Column Match consists of two indices: columns in left and right table and similarity measure (Levevnshtein Similarity, for example).\n"
        "We will use notation [measure(i, j) >= lambda] for Column Similarity Classifier with i'th column of left table, j'th column of right table, similarity measure 'measure' and decision boundary 'lambda'. "
        'Also, notation like [measure("left_col_name", "right_col_name") >= lambda] is valid for ColumnMatch between column with "left_col_name" and "left_col_name" of left and right tables respectively.\n'
    )
    animals_beverages_example()
    print("-" * 50)
    theatre_example()
