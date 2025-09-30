import desbordante
import pandas as pd


from typing import TypedDict
from desbordante.md import ColumnSimilarityClassifier
from desbordante.md.column_matches import Levenshtein, Custom, Equality

# These options allow to print pandas dataframes without skipping columns and wrapping columns to the next line
# Feel free to modify these options if you have issues with the output format
pd.set_option("display.max_columns", None)
pd.set_option("display.width", None)

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"

FLOATING_POINT_DIGITS = 3


class MDParams(TypedDict):
    lhs: list[ColumnSimilarityClassifier]
    rhs: ColumnSimilarityClassifier


def print_md(md):
    md_desc = md.get_description()
    lhs, rhs = md_desc.lhs, md_desc.rhs

    lhs_strings = []

    for lhs_classifier in lhs:
        decision_boundary = round(
            lhs_classifier.decision_boundary, FLOATING_POINT_DIGITS
        )
        column_match_desc = lhs_classifier.column_match_description
        left_column_name = column_match_desc.left_column_description.column_name
        right_column_name = column_match_desc.right_column_description.column_name
        column_match_name = column_match_desc.column_match_name

        lhs_strings.append(
            f"{column_match_name}({left_column_name}, {right_column_name})>={decision_boundary}"
        )

    rhs_decision_boundary = round(rhs.decision_boundary, FLOATING_POINT_DIGITS)
    rhs_column_match_desc = rhs.column_match_description
    rhs_left_column_name = rhs_column_match_desc.left_column_description.column_name
    rhs_right_column_name = rhs_column_match_desc.right_column_description.column_name
    rhs_column_match_name = rhs_column_match_desc.column_match_name

    rhs_string = f"{rhs_column_match_name}({rhs_left_column_name}, {rhs_right_column_name})>={rhs_decision_boundary}"

    print(f'\t[ {" | ".join(lhs_strings)} ] -> {rhs_string}')


def print_results(verifier):
    if verifier.md_holds():
        print(f"{GREEN_CODE}MD holds{DEFAULT_COLOR_CODE}\n")
        return

    highlights = verifier.get_highlights()
    # Also verifier.get_highlights_copy() if you're planning to use highlights separately from verifier

    print(
        f"{RED_CODE}MD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:{DEFAULT_COLOR_CODE}"
    )
    for i, highlight in enumerate(highlights, start=1):
        rhs_desc = highlight.rhs_desc
        column_match_name = rhs_desc.column_match_description.column_match_name
        left_column_name = (
            rhs_desc.column_match_description.left_column_description.column_name
        )
        right_column_name = (
            rhs_desc.column_match_description.right_column_description.column_name
        )
        similarity = round(highlight.similarity, FLOATING_POINT_DIGITS)
        decision_boundary = round(rhs_desc.decision_boundary, FLOATING_POINT_DIGITS)

        print(
            f"{i}. Records ({highlight.left_record_id}, {highlight.right_record_id}) have similarity {similarity}, while dependency states {column_match_name}({left_column_name}, {right_column_name})>={decision_boundary}"
        )

        # Alternative way: print(f"{i}.", highlight.to_string())
        # Or even print(highlight)
    print(
        f"\nDesbordante suggests to use the following right-hand side decision boundary: {round(verifier.get_true_rhs_decision_boundary(), FLOATING_POINT_DIGITS)}.\n"
    )
    print("Thus, the following MD was provided:\n")
    print_md(verifier.get_input_md())
    print("\nand the following MD is suggested:\n")
    print_md(verifier.get_md_suggestion())
    print()


def check_md(table: pd.DataFrame, params: MDParams):
    algo = desbordante.md_verification.algorithms.Default()
    algo.load_data(left_table=table)

    algo.execute(**params)
    print_results(algo)


def animals_beverages_example():
    print("As the first example, let's look at the animals_beverages.csv dataset.\n")

    table_path = "examples/datasets/animals_beverages.csv"
    table = pd.read_csv(table_path)
    print(table, "\n")

    print(
        "Let's try to check if the Matching Dependency\n\n\t[ levenshtein(animal, animal)>=1.0 ] -> levenshtein(diet, diet)>=1.0\n\nholds. "
        "Here, Levenshtein similarity with a decision boundary of 1.0 means values must be exactly equal.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 1)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 1),
    }

    check_md(table, params)
    print(
        "The checked matching dependency used a column similarity classifier with a 1.0 decision boundary on the right side. "
        "However, records with similarity 0.75 — which is below the specified boundary — were found. "
        "Therefore, the checked matching dependency does not hold.\n"
    )
    print(
        "The matching dependency may fail because of typos in the original dataset. "
        "Let's relax both left and right constraints (i.e., require similarity => 0.75) and check the resulting dependency:\n\n"
        "\t[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein(2, 2, 0.0), 0.75)],
        "rhs": ColumnSimilarityClassifier(Levenshtein(3, 3, 0.0), 0.75),
    }

    check_md(table, params)

    print(
        "We can see that the matching dependency\n\n\t[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75\n\nholds.\n"
    )

    print(
        "Now let's look at what happens if we increase the decision boundary on both the left-hand side and the right-hand side. "
        "For example, we'll raise it from 0.75 to 0.76. "
        "First, let's increase the left-hand side decision boundary:\n"
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
        'The values "meat" and "mead" have a Levenshtein similarity of 0.75, which is below the required 0.76; '
        "therefore the matching dependency does not hold.\n"
    )

    print("Let's see whether correcting typos in the dataset changes that.\n")

    table["animal"] = table["animal"].replace({"beer": "bear"})
    table["diet"] = table["diet"].replace({"mead": "meat"})

    print(f"Corrected dataset:\n\n{table}\n")
    print(
        "Now let's re-check the original matching dependency with decision boundaries set to 1.0.\n"
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
    print(table, "\n")

    print(
        "Suppose we already know the following facts about this dataset:\n"
        "1. Each city has a single office, i.e. there is a functional dependency [City] -> OfficeLocation.\n"
        "2. Only managers and chiefs have high-level access, i.e. there is a functional dependency [Position] -> HighLevelAccess.\n"
    )

    print(
        "As we can see, this dataset contains several typos that we will attempt to detect and correct.\n"
    )

    print(
        "Let's start with the functional dependency [City] -> OfficeLocation. "
        "To check it, we examine the following matching dependency:\n\n\t[levenshtein(City, City)>=1.0] -> levenshtein(Office Location, Office Location)>=1.0\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("City", "City", 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("OfficeLocation", "OfficeLocation", 0.0), 1.0
        ),
    }

    check_md(table, params)

    print("The output shows issues in record pairs (0, 1) and (2, 3). Values:")

    for i, (left_index, right_index) in enumerate([(0, 1), (2, 3)], start=1):
        print(
            f"{i}. record {left_index}: \"{table['OfficeLocation'].loc[left_index]}\" — record {right_index}: \"{table['OfficeLocation'].loc[right_index]}\""
        )

    print(
        "\nNow we can see the typos:\n"
        '1. Record 0: missing space in "Main St.17" (should be "Main St. 17").\n'
        '2. Record 2: missing period in "Third St 34" (should be "Third St. 34").\n'
    )

    print("Now let's fix the typos:\n")

    fixed_table = table.copy()

    fixed_table["OfficeLocation"] = fixed_table["OfficeLocation"].replace(
        {"Third St 34": "Third St. 34", "Main St.17": "Main St. 17"}
    )

    print(fixed_table, "\n")

    print("Let's try again:\n")

    check_md(fixed_table, params)

    print(
        "Alternatively, if we consider these typos insignificant for our purposes, we can ignore them. "
        "As Desbordante suggests, we can relax the right-hand decision boundary and check the dependency"
        "\n\n\t[levenshtein(City, City)>=1.0] -> levenshtein(Office Location, Office Location)>=0.9\n\n"
        "over the unmodified table.\n"
    )

    params = {
        "lhs": [ColumnSimilarityClassifier(Levenshtein("City", "City", 0.0), 1.0)],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("OfficeLocation", "OfficeLocation", 0.0), 0.9
        ),
    }

    check_md(table, params)

    print(
        "Let's move on and repeat the procedure for the functional dependency [Position] -> HighLevelAccess. "
        "To check it, we examine the following matching dependency:\n\n\t[levenshtein(Position, Position)>=1.0] -> levenshtein(High Level Access, High Level Access)>=1.0\n"
    )

    params = {
        "lhs": [
            ColumnSimilarityClassifier(Levenshtein("Position", "Position", 0.0), 1.0)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("HighLevelAccess", "HighLevelAccess", 0.0), 1.0
        ),
    }

    check_md(fixed_table, params)

    print(
        "As we can see, there is a discrepancy in records 1 and 4:\n"
        '1. record 1: "yes" — record 4: "Yes"\n'
    )

    print("Now we see the problem. Let's fix it:\n")

    fixed_table.loc[1, "HighLevelAccess"] = "Yes"

    print(fixed_table, "\n")

    print("Let's re-check the matching dependency again:\n")

    check_md(fixed_table, params)

    print(
        "If you look closely, there are still some typos in the dataset:\n"
        '1. Record 0: "manager" should be "Manager".\n'
        '2. Record 5: "yes" was missed during our procedure and should be fixed.\n'
    )

    print(
        "As a result, we can observe two limitations of our approach:\n"
        "1. Typos on the left-hand side of a dependency may go undetected.\n"
        "2. Typos in records with a unique left-hand-side value cannot be detected.\n"
    )

    print(
        "There is an alternative approach to finding typos with MDs. "
        'We will demonstrate it on the "Position" column: '
        "first verify the matching dependency [levenshtein(Position, Position)>=1.0] -> levenshtein(Position, Position)>=1.0, then gradually lower the left-hand side decision boundary until all typos are discovered.\n"
    )

    print(
        "\nVerifying the matching dependency [ levenshtein(Position, Position)>=1.0 ] -> levenshtein(Position, Position)>=1.0:\n"
    )
    params = {
        "lhs": [
            ColumnSimilarityClassifier(Levenshtein("Position", "Position", 0.0), 1.0)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Position", "Position", 0.0), 1.0
        ),
    }
    check_md(fixed_table, params)

    print(
        "\nVerifying Matching Dependency [ levenshtein(Position, Position)>=0.8 ] -> levenshtein(Position, Position)>=1.0:\n"
    )
    params = {
        "lhs": [
            ColumnSimilarityClassifier(Levenshtein("Position", "Position", 0.0), 0.8)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Position", "Position", 0.0), 1.0
        ),
    }
    check_md(fixed_table, params)

    print("Here, with decision boundary 0.8, the first typos were found.\n")

    print(
        "Let's decrease the threshold further and see how it affects the algorithm's output.\n"
    )

    print(
        "Verifying Matching Dependency [ levenshtein(Position, Position)>=0.2 ] -> levenshtein(Position, Position)>=1.0:\n"
    )
    params = {
        "lhs": [
            ColumnSimilarityClassifier(Levenshtein("Position", "Position", 0.0), 0.2)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Position", "Position", 0.0), 1.0
        ),
    }
    check_md(fixed_table, params)

    print(
        "Invoking the algorithm with a decision boundary of 0.8 helped us locate issues in the record pairs (0, 1) and (0, 4). "
        'Record 0 has the value "manager" (uncapitalized) in the "Position" column, so we can fix it as follows:\n'
    )

    fixed_table.loc[0, "Position"] = "Manager"

    print(fixed_table, "\n")

    print(
        "Invoking the algorithm with a decision boundary of 0.2 revealed some additional, but meaningless, patterns. "
        'For example, it considered the value "Clerk" in record 2 and "Chief" in record 5 similar enough.\n'
    )

    print(
        "As a result, we can conclude that this approach allows locating typos without prior knowledge of column dependencies, but requires care in selecting decision boundaries and in analyzing the algorithm's results.\n"
    )


def flights_example():
    print(
        "Now let's examine another example. We will use the flights_dd.csv dataset for this purpose:\n"
    )

    table_path = "examples/datasets/flights_dd.csv"
    table = pd.read_csv(table_path)
    print(table, "\n")

    print(
        "Imagine we want to check that when the departure city and the arrival city are the same, flight times do not differ significantly. "
        "We will treat all Moscow airports as equivalent and need to determine a decision boundary for this purpose.\n"
    )

    print(
        "Let's create a copy of our table and add new Departure and Arrival columns with airport codes removed:\n"
    )

    cities_table = table.copy()
    cities_table["NewDeparture"] = cities_table["Departure"].apply(lambda s: s[:-6])
    cities_table["NewArrival"] = cities_table["Arrival"].apply(lambda s: s[:-6])
    cities_table = cities_table[["Departure", "NewDeparture", "Arrival", "NewArrival"]]

    print(cities_table, "\n")

    print(
        "Now let's check the following matching dependency:\n\n\t[ equality(Departure_new, Departure_new)>=1.0 ] -> levenshtein(Departure, Departure)>=1.0:\n"
    )

    params = {
        "lhs": [
            ColumnSimilarityClassifier(Equality("NewDeparture", "NewDeparture"), 1.0)
        ],
        "rhs": ColumnSimilarityClassifier(
            Levenshtein("Departure", "Departure", 0.0), 1.0
        ),
    }

    check_md(cities_table, params)

    print(
        "As we can see, Desbordante suggests a decision boundary of 0.75 for the Departure column. For the Arrival column, all values are similar.\n"
    )

    print(
        "For duration similarity we use the custom measure normalized_distance, defined as normalized_distance = 1 - |duration_1 - duration_2| / max(Duration), "
        "where duration_1 and duration_2 are values from the Duration column. "
        "This similarity measure is supplied to the verification algorithm. "
        "You can find more examples of custom similarity measures in examples/basic/mining_md.py.\n"
    )
    print(
        "We will try to verify the following matching dependency:\n\n\t[ levenshtein(Departure, Departure)>=0.75 | levenshtein(Arrival, Arrival)>=0.75 ] -> normalized_distance(Duration, Duration)>=1.0\n"
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
        "As a result, we can conclude that durations differ by about 10% for flights with the same departure and arrival cities.\n"
    )


if __name__ == "__main__":
    print(DEFAULT_COLOR_CODE)
    print(
        "This example demonstrates how to verify matching dependencies (MDs) using the Desbordante library. "
        'Matching dependencies are defined in "Efficient Discovery of Matching Dependencies" by Schirmer et al., ACM Transactions on Database Systems (TODS), Vol. 45, No. 3, Article 13, pp. 1–33.\n'
    )
    print(
        "The matching dependency verification algorithm accepts a dependency and determines whether it holds over the specified dataset. "
        "If the dependency does not hold, the algorithm returns a list of exceptions (tuples that violate the MD) and suggests adjustments to the dependency to make it hold.\n"
    )
    print(
        "You can also read about mining matching dependencies in examples/basic/mining_md.py.\n"
    )
    print(
        "To verify a matching dependency, first define column similarity classifiers. "
        "A column similarity classifiers consists of a column match and a decision boundary. "
        "A column match specifies two column identifiers (index or name) — one from the left table and one from the right — and a similarity measure (for example, Levenshtein similarity).\n"
    )
    print(
        'We use the notation [measure(i, j)>=lambda] for a column similarity classifier that specifies the i-th column of the left table, the j-th column of the right table, the similarity measure "measure", and the decision boundary lambda. '
        'The notation [measure("left_col_name", "right_col_name")>=lambda] is also valid for a column match that specifies the columns "left_col_name" and "right_col_name" of the left and right tables, respectively.\n'
    )
    print(
        "Finally, the algorithm is defined over two tables: the left table and the right table. "
        "For simplicity in this example we use a single table (right table = left table). "
        "See the original paper for details on the two-table setting.\n"
    )

    animals_beverages_example()
    print("-" * 100, "\n")
    typos_example()
    print("-" * 100, "\n")
    flights_example()

    print(
        "In conclusion, the matching dependency verification algorithm can be helpful for analyzing data, extracting facts, and finding typos. "
        "It is a powerful pattern but requires experimentation with decision boundaries and similarity measures.\n"
    )
