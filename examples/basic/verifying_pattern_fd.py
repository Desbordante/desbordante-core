import desbordante
import pandas

TABLE_PATH = 'examples/datasets/licenses.csv'
COLOR_CODES = {
    'bold_red': '\u001b[1;31m',
    'bold_green': '\033[1;32m',
    'default': '\033[0m',
    'default_bg': '\033[1;49m',
    'italic': '\033[3m',
    'red_bg':'\033[1;41m',
    'blue':'\033[1;34m'
}

Token = desbordante.pattern_fd_verification.Token
Regex = desbordante.pattern_fd_verification.Regex
Wildcard = desbordante.pattern_fd_verification.Wildcard

def print_clusters(verifier, data, lhs, rhs):
    print(f"Number of clusters violating Pattern FD: {verifier.get_num_error_clusters()}")
    lhs_columns = [data.iloc[:, l] for l in lhs]
    rhs_columns = [data.iloc[:, r] for r in rhs]

    for i, highlight in enumerate(verifier.get_clusters(), start=1):
        violating_rows = highlight.get_violating_rows
        print(f"{COLOR_CODES['blue']}#{i} cluster: {COLOR_CODES['default']}")
        for el in highlight.cluster:
            color = COLOR_CODES['default']
            lhs_values = [col.iloc[el] for col in lhs_columns]
            rhs_values = [col.iloc[el] for col in rhs_columns]
            if el in violating_rows:
                color = COLOR_CODES['red_bg']
            print(f"{color}{el}: {lhs_values} -> {rhs_values}{COLOR_CODES['default']}")


def print_results(table, pattern_fd_verifier, lhs, rhs):
    print(f"{COLOR_CODES['default_bg']}Pattern FD Verification Results:{COLOR_CODES['default']}\n")
    holds = pattern_fd_verifier.pattern_fd_holds()
    coverage = pattern_fd_verifier.get_real_pattern_fd_coverage()
    min_pattern_inclusion = pattern_fd_verifier.get_real_min_pattern_inclusion()
    max_rhs_deviation = pattern_fd_verifier.get_real_max_rhs_deviation()
    print(f"{COLOR_CODES['default_bg']}Pattern FD: {lhs} -> {rhs}")
    if holds:
        print(f"Pattern FD holds: {COLOR_CODES['bold_green']}{holds}{COLOR_CODES['default']}")
    else:
        print(f"Pattern FD holds: {COLOR_CODES['bold_red']}{holds}{COLOR_CODES['default']}")

    print(f"Coverage: {coverage}")
    print(f"Min pattern inclusion: {min_pattern_inclusion}")
    print(f"Max RHS deviation: {max_rhs_deviation}\n")
    print_clusters(pattern_fd_verifier, table, lhs, rhs)

def run_example():
    table = pandas.read_csv(TABLE_PATH, header=0, dtype=str)
    table = table.astype(str)
    print("\nIn this example, let's look at a dataset with data about companies and their licenses.\n")
    print(table)
    print("\nLet's say we want to check if the city of license issuance really determines the license number.\n")
    algo = desbordante.pattern_fd_verification.algorithms.Default()
    algo.load_data(table=table)

    lhs, rhs = [2], [4]
    print(f'This hypothesis will be expressed as a rule: "City" -> "Licensee Number"\n')
    patterns = [{2: Token("GAITHERSBURG", 0), 4: Regex("BBWLHR\\D{3}"), }, 
            {2: Token("GERMANTOWN", 0), 4: Regex("CBWLCC\\D{3}")}]
    print('And we want to check that such patterns are fulfilled:\n')
    print('"GAITHERSBURG" -> "BBWLHR\\D{3}"')
    print('"GERMANTOWN" -> "CBWLCC\\D{3}"\n')

    min_pattern_fd_coverage=10
    max_rhs_deviation=0.2
    min_pattern_inclusion=4

    print(f"""We want each pattern to appear at least 4 times {COLOR_CODES['italic']}(min_pattern_inclusion = 4){COLOR_CODES['default']}, """
    f"""with no more than 20% of them matching by City but differing by Licensee Number {COLOR_CODES['italic']}(max_rhs_deviation = 0.2){COLOR_CODES['default']}.\n"""
    f"""Also, we want the entire functional dependency to cover at least 10 rows {COLOR_CODES['italic']}(min_pattern_fd_coverage = 10){COLOR_CODES['default']}.\n""")

    algo.execute(lhs_indices=lhs, rhs_indices=rhs, 
        min_pattern_fd_coverage=min_pattern_fd_coverage, max_rhs_deviation=max_rhs_deviation, 
        patterns=patterns, min_pattern_inclusion=min_pattern_inclusion)

    print_results(table, algo, lhs, rhs)


if __name__ == '__main__':
    print("This example demonstrates how to validate Pattern Functional Dependencies (Pattern FDs) using the Desbordante library. "
            "The definitions are taken from the paper 'Pattern Functional Dependencies for Data Cleaning'.\n\n"
            "Pattern FD expresses a relationship where a set of attributes X determines attribute Y, "
            "written as (X → Y, T), where T is a pattern table specifying constraints for these attributes.\n"
            "Patterns in T can be either regular expressions defining value structures "
            "or token-position pairs capturing partial value characteristics (unlike CFD which uses full attribute values).\n"
            "Validation verifies whether a user-specified Pattern FD holds in a given dataset based on three key parameters:\n"
            "  • min_pattern_inclusion: minimum number of records a single pattern must cover\n"
            "  • min_pattern_fd_coverage: minimum total number of records covered by the entire Pattern FD\n"
            "  • max_rhs_deviation: maximum allowed violation rate (mismatched Y values for matching X patterns), calculated as the ratio of rows where the left-hand side (X) matches but the right-hand side (Y) doesn't, to all rows where the left-hand side matches.\n"
            "The formula is:\n"
            " max_rhs_deviation = (number of violating rows where X matches but Y doesn't) / (total number of rows where X matches)\n\n"
            "During the validation task, Desbordante not only checks whether a given Pattern FD holds, but it can also 1) adjust the three aforementioned parameters supplied by the user (if their values were overestimated), and 2) present rows that violate this Pattern FD.\n")

    run_example()
    
