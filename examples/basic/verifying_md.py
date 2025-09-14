import desbordante
import pandas as pd


from typing import TypedDict
from desbordante.md import ColumnSimilarityClassifier
from desbordante.md.column_matches import Levenshtein, Custom, Equality

# pd.set_option("display.max_columns", None)

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"


class MDParams(TypedDict):
    lhs: list[ColumnSimilarityClassifier]
    rhs: ColumnSimilarityClassifier


def print_results(verifier):
    if verifier.md_holds():
        print(f"{GREEN_CODE}MD holds{DEFAULT_COLOR_CODE}\n")
        return

    highlights = verifier.get_highlights()
    # Also verifier.get_highlights_copy() if you're planning to use highlights separately from verifier

    print(f"{RED_CODE}MD does not hold due to the following items:{DEFAULT_COLOR_CODE}")
    for i, highlight in enumerate(highlights, start=1):
        rhs_desc = highlight.rhs_desc
        column_match_name = rhs_desc.column_match_description.column_match_name
        left_column_name = (
            rhs_desc.column_match_description.left_column_description.column_name
        )
        right_column_name = (
            rhs_desc.column_match_description.right_column_description.column_name
        )
        decision_boundary = rhs_desc.decision_boundary

        print(
            f"{i}. Rows {highlight.left_table_row} of the left table and "
            f"{highlight.right_table_row} of the right table "
            f"have similarity {highlight.similarity} and, therefore, "
            f"violate right-hand side column similarity classifier "
            f"{column_match_name}({left_column_name}, {right_column_name})>={decision_boundary}"
        )

        # Alternative way: print(f"{i}.", highlight.to_string())
        # Or even print(highlight)
    print(
        f"Desbordante suggests to use the following right-hand side decision boundary: {verifier.get_true_rhs_decision_boundary()}\n"
    )
    print(f"Thus, the following MD was provided:\n  {verifier.get_input_md()}")
    print("and the following MD is suggested:")
    print(f"  {verifier.get_md_suggestion()}")


def check_md(table: pd.DataFrame, params: MDParams):
    algo = desbordante.md_verification.algorithms.Default()
    algo.load_data(left_table=table)

    algo.execute(**params)
    print_results(algo)


def animals_beverages_example():
    print("As the first example, let's look at the animals_beverages.csv dataset\n")

    table_path = "examples/datasets/animals_beverages.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nLet's try to check if the Matching Dependency [ levenshtein(animal, animal)>=1.0 ] -> levenshtein(diet, diet)>=1.0 holds.\n"
        "Levenshtein similarity with the decision boundary equal to 1.0 means that values must be equal.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1),
    }

    check_md(table, params)

    print(
        "\nThe checked Matching Dependency had the column similarity classifier with a 1.0 decision boundary on the right side. "
        "However, records with a 0.75 similarity, which is less than the specified decision boundary, were found. Therefore checked Matching Dependency doesn't hold.\n\n"
        "The Matching Dependency may not hold due to some sort of typo in the original dataset. "
        "Let's relax both left and right constraints (i.e. say that if the similarity between the values in the column is at least 0.75, then values are similar enough) and check this dependency:"
        "\n\n[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

    print(
        "We can see that the Matching Dependency [ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75 holds.\n"
    )

    print(
        "Now let's take a look at what happens if we increase the decision boundary in the left-hand side and the right-hand side. "
        "For example, we will set it to 0.76 instead of 0.75. "
        "First, let's raise the left-hand side decision boundary:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.76)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

    print(
        "As we can see, nothing changed. Now let's raise the right-hand side decision boundary:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.76),
    }

    check_md(table, params)

    print(
        '\nThe values "meat" and "mead" have a similarity of 0.75 according to the Levenshtein similarity measure, but we require the similarity to be at least 0.76, so the Matching Dependency doesn\'t hold.\n'
    )

    print("Let's check if that changes if we correct typos in the dataset")

    table["animal"] = table["animal"].replace({"beer": "bear"})
    table["diet"] = table["diet"].replace({"mead": "meat"})

    print(f"Corrected dataset:\n\n{table}\n")
    print(
        "Now let's check the original Matching Dependency (with 1.0 decision boundaries) again"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1.0),
    }

    check_md(table, params)


def typos_example():
    print("On our next example let's take a view at employee_typos.csv dataset:\n")
    table_path = "examples/datasets/employee_typos.csv"
    table = pd.read_csv(table_path)
    print(table)

    print("\nWe already know some facts about this dataset:")
    print(
        "1. There is only single office in every city, i. e. there is a Functional Dependency [City] -> Office Location."
    )
    print(
        "2. Only managers and chiefs have high level access. For us it means that there is a Functional Dependency [Position] -> High Level Access.\n"
    )

    print(
        "As we see, this dataset contains some typos we'll try to discover and fix.\n"
    )

    print(
        "Let's start from [City] -> Office Location dependency. For such purpose we'll examine Matching Dependency [ levenshtein(City, City)>=1.0 ] -> levenshtein(Office Location, Office Location)>=1.0:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("City", "City", 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Office Location", "Office Location", 0.0), 1.0
        ),
    }

    check_md(table, params)

    print(
        "\nFrom the output we see that there are issues in records pairs (0, 1) and (2, 3). Let's take a look at values:"
    )

    for i, (left_index, right_index) in enumerate([(0, 1), (2, 3)], start=1):
        print(
            f"{i}. record {left_index}: \"{table['Office Location'].loc[left_index]}\", record {right_index}: \"{table['Office Location'].loc[right_index]}\""
        )

    print("\nNow we can see all typos:")

    print('1. In record 0 there is a missing space in value "Main St.17"')
    print('2. In record 2 there is a missing dot in value "Third St 34"\n')

    print("Now let's fix the typos and try again:\n")

    fixed_table = table.copy()

    fixed_table["Office Location"] = fixed_table["Office Location"].replace(
        {"Third St 34": "Third St. 34", "Main St.17": "Main St. 17"}
    )

    print(fixed_table, "\n")

    check_md(fixed_table, params)

    print(
        "Also, there is another approach. If we suppose that such typos don't affect clarity of our data a lot, we can just ignore them. "
        "As Desbordante suggests, we can use new decision boundary and examine Matching Dependency [ levenshtein(City, City)>=1.0 ] -> levenshtein(Office Location, Office Location)>=0.9:\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("City", "City", 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Office Location", "Office Location", 0.0), 0.9
        ),
    }

    check_md(table, params)

    print(
        "\nLet's move onward and repeat this procedure for Functional Dependency [Position] -> High Level Access. "
        "For such purpose we'll examine Matching Dependency [ levenshtein(Position, Position)>=1.0 ] -> levenshtein(High Level Access, High Level Access)>=1.0:\n"
    )

    params = {
        "lhs": [
            ColumnSimilarityClassifier(Levenshtein("Position", "Position", 0.0), 1.0)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("High Level Access", "High Level Access", 0.0), 1.0
        ),
    }

    check_md(fixed_table, params)

    print("\nAs we see, there is a problem in records 1 and 4:")
    print(
        f"1. record 1: \"{fixed_table['High Level Access'].loc[1]}\", record 4: \"{fixed_table['High Level Access'].loc[4]}\"\n"
    )

    print(
        "Now we see the problem. Let's fix it and take a look at the dataset and the Matching Dependency again:\n"
    )

    fixed_table.loc[1, "High Level Access"] = "Yes"

    print(fixed_table, "\n")

    check_md(fixed_table, params)

    print(
        "\nIf you're attentive enough, you can see that there are still some typos in the dataset:"
    )
    print('1. In record 0 value "manager" should be replaced with "Manager"')
    print('2. In record 5 value "yes" was not found during our procedure')

    print("\nAs a result, we can see problems of our approach:")
    print("1. Typos in left-hand side of dependency cannot be found")
    print("2. Typos in records with unique left-hand side cannot be found\n")

    print(
        "There is another procedure for searching typos in the dataset. "
        'For example, let\'s examine column "Position". '
        "We'll start from verifying Matching Dependency [ levenshtein(Position, Position)>=1.0 ] -> levenshtein(Position, Position)>=1.0 and step by step decrease left-hand side decision bondary "
        "until we'll find all the typos."
    )

    for lam in [1.0, 0.8, 0.2]:
        print(
            f"\nVerifying Matching Dependency [ levenshtein(Position, Position)>={lam} ] -> levenshtein(Position, Position)>=1.0:\n"
        )
        params = {
            "lhs": [
                ColumnSimilarityClassifier(
                    Levenshtein("Position", "Position", 0.0), lam
                )
            ],
            "rhs": ColumnSimilarityClassifier(
                Levenshtein("Position", "Position", 0.0), 1.0
            ),
        }

        check_md(fixed_table, params)

    print(
        "Decision boundary of 0.8 helped us to find problems in records pairs (0, 1) and (0, 4). "
        'Record 0 has value "manager" in column "Position", so we can fix it:\n'
    )

    fixed_table.loc[0, "Position"] = "Manager"

    print(fixed_table, "\n")

    print(
        "Algorithm with decision boundary of 0.2 found some wrong patterns. "
        'For example, it counted that values "Clerk" in record 2 and "Chief" in record 5 are similar enough.\n'
    )

    print(
        "As a result we can conclude that such approach allows to find typos without information about any dependency between columns in data, "
        "but requires accuracy in selecting decision boundaries and analyzing the results of the algorithm.\n"
    )


def flights_example():
    print(
        "Now let's examine another example. We will use the flights_dd.csv dataset for this purpose:\n"
    )

    table_path = "examples/datasets/flights_dd.csv"
    table = pd.read_csv(table_path)
    print(table)

    print(
        "\nLet's imagine we want to check that if the departure city and the arrival city are the same, flight times don't differ a lot. "
        "\nWe want to consider all Moscow's airports as similar. We need to find a decision boundary for this purpose.\n"
    )

    print(
        "Let's create copy of our table and remove the airport's literals from the Departure and Arrival columns:\n"
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
        "As we see, Desbordante suggests to use the decision boundary of 0.75 for the Departure column. For the Arrival column everything is similar.\n"
    )

    print(
        "For duration similarity measure in our example we'll use normalized_distance. normalized_distance(Duration, Duration) is a custom similarity measure we provided to the verification algorithm. "
        "It is equal to 1 - abs(duration_1 - duration_2) / max(Duration), where duration_1 and duration_2 are the values from the Duration column. "
        "You can see more examples of custom similarity measures in examples/basic/mining_md.py.\n\n"
        "We will try to verify the Matching Dependency [ levenshtein(Departure, Departure)>=0.75 | levenshtein(Arrival, Arrival)>=0.75 ] -> normalized_distance(Duration, Duration)>=1.0\n"
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
        "The algorithm provided us with a new decision boundary for a right-hand side. Let's relax it to 0.895:\n"
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
        "As a result, we can conclude that duration differs about 10 percent for flights with the same departure/arrival cities."
    )


if __name__ == "__main__":
    print(DEFAULT_COLOR_CODE)
    print(
        "This example demonstrates how to verify Matching dependencies (MD) which are defined in 'Efficient Discovery of Matching Dependencies' "
        "by Schirmer et al. published in ACM Transactions on Database Systems (TODS), Volume 45, Issue 3 Article No.: 13, Pages 1 - 33 using the Desbordante library.\n"
    )
    print(
        "The Matching Dependency verification algorithm accepts the left-hand side and right-hand side and determines if the specified dependency holds. "
        "Also, in case the dependency doesn't hold, the algorithm returns a list of exceptions (tuples that violate the given MD) and suggests how to adjust the dependency.\n"
    )
    print(
        "You can also read about mining Matching Dependencies in examples/basic/mining_md.py\n"
    )

    print(
        "To verify a Matching Dependency, we must first define Column Similarity Classifiers for the data. "
        "A Column Similarity Classifier consists of a Column Match and a decision boundary. "
        "A Column Match consists of two column identifiers (index or name) for the columns in the left and right tables and a similarity measure (Levenshtein Similarity, for example).\n\n"
        "We will use the notation [measure(i, j) >= lambda] for a Column Similarity Classifier that specifies the i-th column of the left table, the j-th column of the right table, the similarity measure 'measure' and the decision boundary 'lambda'. "
        'Notation like [measure("left_col_name", "right_col_name") >= lambda] is also valid for a Column Match specifying the columns "left_col_name" and "right_col_name" of the left and right tables respectively.\n'
    )

    print(
        "Also, the algorithm is defined over two tables: the left table and the right table. "
        "For simplicity of the example, we will use only one table (right table = left table). "
        "You can read more about the two tables in the original article."
    )

    animals_beverages_example()
    print("-" * 100, "\n")
    typos_example()
    print("-" * 100, "\n")
    flights_example()

    print(
        "In conclusion, Matching Dependencies verification algorithm can be helpful in analyzing data, extracting facts and searching typos. "
        "It is powerful primitive, but requires to experiment with decision boundaries and similarity measures."
    )
