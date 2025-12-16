import desbordante
import pandas

TABLE_PATH = 'examples/datasets/cfd_verification_datasets/city.csv'
TABLE_PATH_FIXED = 'examples/datasets/cfd_verification_datasets/city_fixed.csv'
COLOR_CODES = {
    'bold_red': '\u001b[1;31m',
    'bold_green': '\033[1;32m',
    'default': '\033[0m',
    'default_bg': '\033[1;49m',
    'red_bg':'\033[1;41m',
    'green_bg':'\033[1;42m',
    'blue':'\033[1;34m'
}


def print_clusters(verifier, data, lhs, rhs):
    print(f"Number of clusters violating FD: {verifier.get_num_clusters_violating_cfd()}")
    lhs_columns = [data[col] for col, _ in lhs]
    rhs_columns = [data[rhs[0]]]

    for i, highlight in enumerate(verifier.get_highlights(), start=1):
        violating_rows = highlight.get_violating_rows
        print(f"{COLOR_CODES['blue']} #{i} cluster: {COLOR_CODES['default']}")
        for el in highlight.cluster:
            color = COLOR_CODES['default']
            lhs_values = [col.iloc[el] for col in lhs_columns]
            rhs_values = [col.iloc[el] for col in rhs_columns]
            if el in violating_rows:
                color = COLOR_CODES['red_bg']
            print(f"{color}{el}: {lhs_values} -> {rhs_values}{COLOR_CODES['default']}")


def print_results(table, cfd_verifier, lhs, rhs):
    holds = cfd_verifier.cfd_holds()
    support = cfd_verifier.get_real_support()
    confidence = cfd_verifier.get_real_confidence()
    print(f"{COLOR_CODES['default_bg']}CFD: {lhs} -> {rhs}")
    if holds:
        print(f"CFD holds: {COLOR_CODES['bold_green']}{holds}{COLOR_CODES['default']}")
    else:
        print(f"CFD holds: {COLOR_CODES['bold_red']}{holds}{COLOR_CODES['default']}")

    print(f"Support: {support}")
    print(f"Confidence: {confidence:.2f}")
    print_clusters(cfd_verifier, table, lhs, rhs)

def print_table_with_highlight(table, indices_to_highlight):
    if table.empty:
        print(table.to_string())
    else:
        df_as_string_lines = table.to_string().splitlines()
        print(df_as_string_lines[0])
        data_lines = df_as_string_lines[1:]

        for i, line_text in enumerate(data_lines):
            if table.index[i] in indices_to_highlight:
                print(f"{COLOR_CODES['green_bg']}{line_text}{COLOR_CODES['default']}")
            else:
                print(line_text)

def scenario_incorrect_data():
    table = pandas.read_csv(TABLE_PATH)
    print("\nIn the first example, let's look at a dataset containing real estate properties in different cities.\n")
    print(table)
    print("\nLet's say we want to check whether highly priced buildings in Los Angeles are determined by (depend on) a specific building type.\n")
    algo = desbordante.cfd_verification.algorithms.Default()
    algo.load_data(table=table)

    lhs, rhs = [("City", "Los Angeles"), ("BuildingType", "_")], ("BuildingCost", "high")
    print(f'This hypothesis will be expressed as a rule: [("City", "Los Angeles"), ("BuildingType", "_")] -> ("BuildingCost", "high")\n')
    algo.execute(cfd_rule_left=lhs, cfd_rule_right=rhs, cfd_minconf = 1)

    print_results(table, algo, lhs, rhs)
    print("\nWe can see that the rule is violated in line 6, which may indicate incorrect data entry. Let's fix them.\n") #

    fix_condition = (table["City"] == "Los Angeles") & (table["BuildingCost"] != "high")
    indices_to_highlight = table[fix_condition].index.tolist()
    table.loc[fix_condition, "BuildingCost"] = "high"
    table.to_csv(TABLE_PATH_FIXED, index=False)

    print_table_with_highlight(table, indices_to_highlight)

    table = pandas.read_csv(TABLE_PATH_FIXED)
    algo = desbordante.cfd_verification.algorithms.Default()
    algo.load_data(table=table)

    lhs, rhs = [("City", "Los Angeles"), ("BuildingType", "_")], ("BuildingCost", "high")
    algo.execute(cfd_rule_left=lhs, cfd_rule_right=rhs, minconf = 1)

    print_results(table, algo, lhs, rhs)


if __name__ == '__main__':
    print("This example demonstrates how to validate Conditional Functional Dependencies (CFDs) using the Desbordante library.\n"
      "The definitions are taken from the paper 'Revisiting Conditional Functional Dependency Discovery: Splitting the “C” from the “FD”' (ECML PKDD 2018).\n"
      "CFD expresses a relationship in which a subset of attributes X defines Y, "
      "written as (X -> Y, t), where t is a certain template tuple.\n"
      "A template tuple t is a tuple where each attribute can either have a fixed constant value or a wildcard symbol ('_'), "
      "allowing for generalization across different data records.\n"
      "Validation checks whether a user-specified CFD holds for a given dataset, based on the specified values for support and confidence thresholds. "
      "Support is the quantity of records satisfying the condition, "
      "and confidence is the fraction of records where Y occurs given X.\n"
      "Desbordante detects CFD violations and classifies records based on rule compliance.\n")


    scenario_incorrect_data()

    print('\nThats all for CFD validation. Desbordante is also capable of CFD discovery, which is discussed in "mining_cfd.py".')
