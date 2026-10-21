"""Explain and validate a Pattern FD on the licenses dataset."""

from pathlib import Path

import desbordante
import pandas


TABLE_PATH = Path(__file__).resolve().parents[1] / "datasets" / "licenses.csv"
CITY = 2
ZIP = 3
LICENSE_NUMBER = 4

MIN_PATTERN_FD_COVERAGE = 10
MIN_PATTERN_INCLUSION = 4
MAX_RHS_DEVIATION = 0.2

Token = desbordante.pattern_fd_verification.Token
Regex = desbordante.pattern_fd_verification.Regex
Wildcard = desbordante.pattern_fd_verification.Wildcard


def explain(*paragraphs):
    print("\n\n".join(paragraphs), end="\n\n")


def print_introduction():
    explain(
        "Pattern FD validation in Desbordante. This example is based on "
        "'Pattern Functional Dependencies for Data Cleaning' by Abdulhakim Qahtan, "
        "Nan Tang, Mourad Ouzzani, Yang Cao, and Michael Stonebraker, "
        "Proceedings of the VLDB Endowment (PVLDB), 13(5), 2020. "
        "https://www.vldb.org/pvldb/vol13/p684-qahtan.pdf",
        "Imagine that license numbers issued in a city should have a particular prefix "
        "followed by three digits. If one number is missing a letter, we can flag that "
        "row without knowing the correct number in advance. This is validation: we "
        "supply the dependency and its patterns for Desbordante to check.",
        "Formally, a Pattern FD is written (X -> Y, T): X and Y are sets of columns, "
        "and T is a pattern tableau. Each row of T assigns patterns to columns in "
        "X and Y. Both sides may contain several columns; lhs_indices and rhs_indices "
        "are lists of their zero-based column indices. For example, lhs_indices=[2, 3] "
        "uses City and Zip, while rhs_indices=[0, 4] checks Licensee Name and "
        "Licensee Number. Here we specify a pattern for every listed column. "
        "The running example uses one column on each side to keep the output readable.",
        r"The paper draws a bar over a constrained part: matching rows must agree on "
        r"that part. In Desbordante's Regex(pattern), write <...> instead. For example, "
        r"Regex(r'<\D{3}>\D{2}') accepts five digits and compares the first three. "
        r"The bar and angle brackets are notation for comparison, not characters in "
        r"the data. Regex(r'BBWLHR\D{3}') checks only the value's structure: the "
        r"literal prefix BBWLHR followed by three digits. Here \D means a digit in "
        r"Desbordante's pattern language, unlike Python's regex escape \D.",
        "Token(text, position) specifies a token-position pair: the given text must "
        "appear at a zero-based position. Depending on the column's mode, position "
        "counts tokens or characters; in this example the whole city name starts at "
        "position 0. The paper's bottom symbol (\u22a5) is Wildcard() in Python. It "
        "can be placed in a tableau row as {ZIP: Wildcard()}. It accepts any value; "
        "when values are compared within a group, their complete values are used. "
        "Regex also supports \\A (any character), \\U (uppercase), "
        "\\L (lowercase), and \\S (a non-word symbol), as well as repetitions such "
        "as {N}, * and +. Use raw Python strings for patterns containing backslashes.",
    )


def print_options():
    print("Options available to this verifier:")
    print("  load_data(table=...): the input table, here a pandas DataFrame")
    print("  load_data(is_null_equal_null=...): whether nulls compare equal (default: True)")
    print("  execute(lhs_indices=...): zero-based indices of the LHS columns")
    print("  execute(rhs_indices=...): zero-based indices of the RHS columns")
    print("  execute(patterns=...): the pattern tableau, a list of column-to-pattern maps")
    print("  execute(min_pattern_fd_coverage=...): minimum distinct rows matching a tableau LHS")
    print("  execute(min_pattern_inclusion=...): minimum LHS matches for every tableau row")
    print("  execute(max_rhs_deviation=...): maximum allowed RHS violation fraction\n")


def violating_row_indices(verifier):
    return {
        row
        for highlight in verifier.get_clusters()
        for row in highlight.get_violating_rows
    }


def print_results(title, table, verifier, lhs_indices, rhs_indices):
    lhs_names = [table.columns[index] for index in lhs_indices]
    rhs_names = [table.columns[index] for index in rhs_indices]
    print(f"{title}: {lhs_names} -> {rhs_names}")
    print(f"  Pattern FD holds: {verifier.pattern_fd_holds()}")
    print(f"  Observed coverage: {verifier.get_real_pattern_fd_coverage()}")
    print(f"  Observed minimum pattern inclusion: {verifier.get_real_min_pattern_inclusion()}")
    print(f"  Observed maximum RHS deviation: {verifier.get_real_max_rhs_deviation():.2f}")
    print(f"  Violating rows: {verifier.get_num_error_rows()}")
    print(f"  Clusters with violations: {verifier.get_num_error_clusters()}")

    for number, highlight in enumerate(verifier.get_clusters(), start=1):
        violating_rows = set(highlight.get_violating_rows)
        print(f"  Cluster {number}:")
        for row in sorted(highlight.cluster):
            lhs_values = table.iloc[row, lhs_indices].tolist()
            rhs_values = table.iloc[row, rhs_indices].tolist()
            label = "  <-- violation" if row in violating_rows else ""
            print(f"    row {row}: {lhs_values} -> {rhs_values}{label}")
    print()


def verify(table, lhs_indices, rhs_indices, pattern_tableau, max_rhs_deviation, title):
    verifier = desbordante.pattern_fd_verification.algorithms.Default()
    verifier.load_data(table=table, is_null_equal_null=True)
    verifier.execute(
        lhs_indices=lhs_indices,
        rhs_indices=rhs_indices,
        patterns=pattern_tableau,
        min_pattern_fd_coverage=MIN_PATTERN_FD_COVERAGE,
        min_pattern_inclusion=MIN_PATTERN_INCLUSION,
        max_rhs_deviation=max_rhs_deviation,
    )
    print_results(title, table, verifier, lhs_indices, rhs_indices)
    return verifier


def run_example():
    print_introduction()
    print_options()

    table = pandas.read_csv(TABLE_PATH, dtype=str, keep_default_na=False)
    explain(
        "The licenses dataset contains the following rows. Row numbers on the left "
        "are zero-based and will also appear in the validation results:"
    )
    print(table.to_string(), end="\n\n")

    city_prefixes = (
        ("GAITHERSBURG", "BBWLHR"),
        ("GERMANTOWN", "CBWLCC"),
    )
    lhs_indices = [CITY]
    rhs_indices = [LICENSE_NUMBER]

    # The tableau is separate from the embedded FD, City -> Licensee Number.
    pattern_tableau = [
        {CITY: Token(city, 0), LICENSE_NUMBER: Regex(prefix + r"\D{3}")}
        for city, prefix in city_prefixes
    ]

    explain(
        "Our proposed dependency is City -> Licensee Number. The pattern tableau "
        "has two rows: for GAITHERSBURG, a license number must start with BBWLHR; "
        "for GERMANTOWN, it must start with CBWLCC. Both prefixes are followed by "
        "exactly three digits. The Python names Token and Regex correspond to the "
        "two kinds of patterns in this tableau. Other cities, such as ROCKVILLE, "
        "are outside these tableau rows."
    )
    for city, prefix in city_prefixes:
        pattern_text = prefix + r"\D{3}"
        print(f"  City = Token({city!r}, 0) -> Licensee Number = Regex({pattern_text!r})")
    print()

    explain(
        "We require at least four LHS matches for each tableau row "
        "(min_pattern_inclusion=4) and ten distinct LHS-matching rows overall "
        "(min_pattern_fd_coverage=10). For each tableau row, RHS deviation is "
        "its violating rows divided by its LHS-matching rows. The reported "
        "max_rhs_deviation is the largest such fraction; the allowed limit is 0.2. "
        "Desbordante reports the observed values alongside the user-supplied "
        "limits; it does not change those limits."
    )

    first = verify(
        table, lhs_indices, rhs_indices, pattern_tableau,
        MAX_RHS_DEVIATION, "Results on the original data"
    )
    explain(
        "In the original data, both city patterns cover five rows. Two GERMANTOWN "
        "license numbers lack a letter in the expected CBWLCC prefix, so that "
        "tableau row has deviation 2/5 = 0.4. This exceeds the allowed 0.2.",
        "To illustrate the result, change the two highlighted values in a copy "
        "of the table so they match the expected pattern, then run the same "
        "validation again. These are illustrative values, not verified license "
        "numbers. The CSV file is unchanged."
    )

    modified = table.copy()
    replacements = {2: "CBWLCC021", 5: "CBWLCC002"}
    for row, replacement in replacements.items():
        previous = modified.iat[row, LICENSE_NUMBER]
        modified.iat[row, LICENSE_NUMBER] = replacement
        print(f"  row {row}: {previous} -> {replacement}")
    print()

    modified_result = verify(
        modified, lhs_indices, rhs_indices, pattern_tableau,
        MAX_RHS_DEVIATION, "Results after changing the highlighted values"
    )
    removed = sorted(violating_row_indices(first) - violating_row_indices(modified_result))
    print(f"Previously highlighted rows no longer highlighted: {removed}\n")

    explain(
        "The same data can also be checked with a different threshold. If we "
        "allow max_rhs_deviation=0.4 on the original table, "
        "pattern_fd_holds() becomes true, but the two highlighted rows remain. "
        "Changing a threshold changes the acceptance decision, not the data."
    )
    verify(
        table, lhs_indices, rhs_indices, pattern_tableau,
        0.4, "Results on original data with a relaxed limit"
    )

    # A second LHS attribute is checked along with City. The RHS remains unchanged.
    two_lhs_tableau = [
        {
            CITY: Token(city, 0),
            ZIP: Regex(r"\D{5}"),
            LICENSE_NUMBER: Regex(prefix + r"\D{3}"),
        }
        for city, prefix in city_prefixes
    ]
    explain(
        "Finally, use two LHS columns: City and Zip. A row must satisfy both "
        "the city token and the five-digit Zip pattern to be included. Since "
        "Regex(r'\\D{5}') has no <...> part, the complete Zip values are used "
        "when grouping LHS rows. This example has two LHS columns, and the API "
        "allows more; rhs_indices can likewise list multiple RHS columns, with "
        "their patterns included in each tableau row."
    )
    verify(
        modified, [CITY, ZIP], rhs_indices, two_lhs_tableau,
        MAX_RHS_DEVIATION, "Results with two LHS columns"
    )


if __name__ == "__main__":
    run_example()
