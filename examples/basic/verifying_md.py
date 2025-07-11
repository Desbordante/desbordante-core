import desbordante
import pandas as pd

from typing import TypedDict
from desbordante.md import ColumnSimilarityClassifier
from desbordante.md.column_matches import Levenshtein, Custom, Equality

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
    print(f"Thus, the following MD was provided:\n  {verifier.get_input_md()}")
    print("and the following MDs are suggested:")
    for md in verifier.get_md_suggestions():
        print(f"  {md.to_string_active()}")
    print()


def check_md(table: pd.DataFrame, params: MDParams):
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
        "\nLet's try to check if MD [ levenshtein(animal, animal)>=1.0 ] -> levenshtein(diet, diet)>=1.0 holds.\n"
        "Levenshtein similarity with decision boundary equal to 1.0 means that values must be equal.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1),
    }

    check_md(table, params)

    print(
        "We passed column similarity classifier with decision boundary equals to 1.0, but rows with similarity equal to 0.75 were found. Similarity is less than decision boundary, so MD doesn't hold.\n"
        "MD may not hold due to some sort of typo in original dataset.\n"
        "Let's relax both our constraints (i.e. say that if column's similarity measure is at least 0.75, then row values are similar enough) and check this dependency:\n[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

    print(
        "We can see that MD [ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75 holds.\n"
    )

    print(
        "Now let's take a look what happens if we increase decision boundary of left-hand side and right-hand side. "
        "For example, we will take 0.76 instead of 0.75. "
        "First, let's raise left-hand side decision boundary:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.76)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

    print(
        "As we can see, nothing had changed. Now let's raise right-hand side decision boundary:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.76),
    }

    check_md(table, params)

    print(
        'Values "meat" and "mead" have Levenshtein similarity measure equal to 0.75, but we accept similarity measure at least 0.76, so MD doesn\'t hold.\n'
    )

    print("Let's check what changes if we correct typos in dataset")

    table["animal"] = table["animal"].replace({"beer": "bear"})
    table["diet"] = table["diet"].replace({"mead": "meat"})

    print(f"Corrected dataset:\n\n{table}\n")
    print("Now let's check the original MD (with 1.0 decision boundaries) again")

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1.0),
    }

    check_md(table, params)


def flights_example():
    print(
        "Now let's examine an another example.\nWe will use flights_dd.csv dataset for such purpose:\n"
    )

    table_path = "examples/datasets/flights_dd.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nLet's imagine we want to check if departure city and arrival city are the same, flight's time doesn't differ a lot. "
        "\nWe want to consider all Moscow's airports as similar values. We need to find decision boundary for such purpose.\n"
    )

    print(
        "Let's create our table's copy and remove airport's literals from Departure and Arrival columns:\n"
    )

    cities_table = table.copy()
    cities_table["Departure_new"] = cities_table["Departure"].apply(lambda s: s[:-6])
    cities_table["Arrival_new"] = cities_table["Arrival"].apply(lambda s: s[:-6])
    cities_table = cities_table[
        ["Departure", "Departure_new", "Arrival", "Arrival_new"]
    ]

    print(cities_table)

    print(
        "\nNow let's check [ equality(Departure_new, Departure_new)>=1.0 ] -> levenshtein(Departure, Departure)>=1.0:\n"
    )

    params = {
        "lhs": [
            ColumnSimilarityClassifier(Equality("Departure_new", "Departure_new"), 1.0)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Departure", "Departure", 0.0), 1.0
        ),
    }

    check_md(cities_table, params)

    print(
        "As we see, Desbordante suggests to use decision boundary of 0.75 for Departure column to count values similar enough if such airports represent the same city. For Arrival column everything is similar.\n"
    )

    print(
        "For duration similarity in our example measure we'll use normalized_distance. normalized_distance(Duration, Duration) is a custom similarity measure provided to algorithm by ourselves. It is equal to 1 = abs(duration_1 - duration_2) / max(Duration), where duration_1 and duration_2 are values from Duration column. "
        "More examples on custom similarity measures you can see in examples/basic/mining_md.py\n"
        "We will try to discover MD [ levenshtein(Departure, Departure)>=0.75 | levenshtein(Arrival, Arrival)>=0.75 ] -> normalized_distance(Duration, Duration)>=1.0\n"
    )
    max_duration = max(table["Duration"])

    params = {
        "lhs": [
            ColumnSimilarityClassifier(
                Levenshtein("Departure", "Departure", 0.0), 0.75
            ),
            ColumnSimilarityClassifier(Levenshtein("Arrival", "Arrival", 0.0), 0.75),
        ],
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

    print(
        "Algorithm provided us with new decision boundary for right-hand side. Let's relax it to 0.895:\n"
    )

    params = {
        "lhs": [
            ColumnSimilarityClassifier(
                Levenshtein("Departure", "Departure", 0.0), 0.75
            ),
            ColumnSimilarityClassifier(Levenshtein("Arrival", "Arrival", 0.0), 0.75),
        ],
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
            0.895,
        ),
    }

    check_md(table, params)

    print(
        "As we see, duration doesn't differ a lot in terms of normalized_distance similarity."
    )


if __name__ == "__main__":
    print(DEFAULT_COLOR_CODE)
    print(
        "This example demonstrates how to validate Matching dependencies (MD) from 'Efficient Discovery of Matching Dependencies' "
        "by Schirmer et al. published at ACM Transactions on Database Systems (TODS), Volume 45, Issue 3 Article No.: 13, Pages 1 - 33. using the Desbordante library.\n"
    )
    print(
        "Matching dependencies verification algorithm accepts left-hand side and right-hand side and returns if such dependency holds. "
        "Also in case if dependency doesn't hold, algorithm also returns list of highlights and suggests how to adjust dependency.\n"
    )
    print(
        "You can also read about mining Matching dependencies in examples/basic/mining_md.py\n"
    )

    print(
        "To verify Matching dependency, firstly we must define Column Similarity Classifiers for tables. "
        "Column Similarity Classifier consists of Column Match and decision boundary. "
        "Column Match consists of two indices: columns in left and right table and similarity measure (Levevnshtein Similarity, for example).\n\n"
        "We will use notation [measure(i, j) >= lambda] for Column Similarity Classifier with i'th column of left table, j'th column of right table, similarity measure 'measure' and decision boundary 'lambda'. "
        'Also, notation like [measure("left_col_name", "right_col_name") >= lambda] is valid for Column Match between columns with names "left_col_name" and "right_col_name" of left and right tables respectively.\n'
    )

    animals_beverages_example()
    print("-" * 100, "\n")
    flights_example()
