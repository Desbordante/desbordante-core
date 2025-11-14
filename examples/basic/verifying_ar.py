import desbordante
import pandas
from tabulate import tabulate

TABLE_TABULAR = 'examples/datasets/rules_book_rows.csv'
TABLE_FIXED_TABULAR = 'examples/datasets/ar_verification_datasets/rules_book_rows_fixed.csv'
TABLE_SINGULAR = 'examples/datasets/rules_book.csv'
COLOR_CODES = {
    'bold_red': '\u001b[1;31m',
    'bold_green': '\033[1;32m',
    'bold_yellow': '\033[1;33m',
    'bold_blue': '\033[1;34m',
    'green': '\033[32m',
    'yellow': '\033[33m',
    'default': '\033[0m',
    'blue_bg': '\033[1;46m',
    'default_bg': '\033[1;49m'
}


def print_table(table, title=None):
    if title is not None:
        print(title)

    print(tabulate(table, tablefmt='pipe', stralign='center'))
    print()


def print_results(ar_verifier, supp, conf):
    actual_supp, actual_conf = ar_verifier.get_real_support(), ar_verifier.get_real_confidence()
    if ar_verifier.ar_holds():
        print(f'AR holds: {COLOR_CODES["bold_green"]}{ar_verifier.ar_holds()}{COLOR_CODES["default"]}')
        print('actual confidence: ', end='')
        print(COLOR_CODES['bold_green'], end='')
        print('{:1.2f}'.format(actual_conf),
              COLOR_CODES['default'])
        print('actual support: ', end='')
        print(COLOR_CODES['bold_green'], end='')
        print('{:1.2f}'.format(actual_supp),
              COLOR_CODES['default'])
    else:
        print(f'AR holds: {COLOR_CODES["bold_red"]}{ar_verifier.ar_holds()}{COLOR_CODES["default"]}')
        print('actual confidence: ', end='')
        if actual_conf >= conf:
            print(COLOR_CODES['bold_green'], end='')
        elif actual_conf + 0.5 > conf:
            print(COLOR_CODES['bold_yellow'], end='')
        else:
            print(COLOR_CODES['bold_red'], end='')

        print('{:1.2f}'.format(actual_conf),
              COLOR_CODES['default'])
        print('actual support: ', end='')

        if actual_supp >= supp:
            print(COLOR_CODES['bold_green'], end='')
        elif actual_supp + 0.5 > supp:
            print(COLOR_CODES['bold_yellow'], end='')
        else:
            print(COLOR_CODES['bold_red'], end='')

        print('{:1.2f}'.format(actual_supp),
              COLOR_CODES['default'])
        print(f'Total number of transactions satisfying AR: {ar_verifier.get_num_transactions_satisfying_ar()}')
        print(f'Total number of transactions violating AR: {ar_verifier.get_num_transactions_violating_ar()}')
        print(f'Number of clusters violating AR: {ar_verifier.get_num_clusters_violating_ar()}')
        print('Clusters violating AR:')
        clusters_violating_ar = ar_verifier.get_clusters_violating_ar()
        for priority, cluster in clusters_violating_ar.items():
            print(f'{priority} : {cluster}')
    print()


def scenario_tabular():
    #case 1
    table = pandas.read_csv(TABLE_TABULAR, header=None)
    print("As the first example, let's look at the dataset containing receipts "
          'from some supermarket using input_format="tabular". In this '
          'format, each table row lists all items participating in the same '
          'transaction. Note that, in this table, '
          'some rows may be shorter than others.\n')
    print_table(table)
    algo = desbordante.ar_verification.algorithms.Default()
    algo.load_data(table=(TABLE_TABULAR, ',', False), input_format='tabular')
    supp, conf = 0.2, 0.6
    print(f"Let's check if the rule [Bread] -> [Butter] holds for the given confidence {conf} and support {supp}:\n")
    algo.execute(lhs_rule=["Bread"], rhs_rule=["Butter"], minsup=supp, minconf=conf)
    print_results(algo, supp, conf)
    print("\nYou can see that the actual values of support and confidence "
          "are different from those requested. Let's examine the clusters that exhibit this violation: "
          "transaction #2 contains the entire left side of the association rule but is missing the right side, "
          "indicating a potential error:\n")
    print(table.iloc[2].to_list())
    print()
    print("Assume we are certain that the data is corrupted and that "
          "the transaction #2 should contain 'Butter'. We will fix this record and re-validate the rule.")

    algo = desbordante.ar_verification.algorithms.Default()
    algo.load_data(table=(TABLE_FIXED_TABULAR, ',', False), input_format='tabular')
    algo.execute(lhs_rule=["Bread"], rhs_rule=["Butter"], minsup=supp, minconf=conf)
    table = pandas.read_csv(TABLE_FIXED_TABULAR, header=None)
    print_table(table)
    print_results(algo, supp, conf)
    print("Now, with the corrected data, we see the association rule [Bread] -> [Butter] "
          "holds with the given support and confidence values.\n\n")

    # case 2
    algo = desbordante.ar_verification.algorithms.Default()
    algo.load_data(table=(TABLE_TABULAR, ',', False), input_format='tabular')
    supp, conf = 0.60, 0.60
    table = pandas.read_csv(TABLE_TABULAR, header=None)
    print("Let's look at a rule whose left side consists of several elements. "
          f"Rule [Milk, Yogurt] -> [Eggs] with support = {supp}, confidence = {conf}")
    print_table(table)
    algo.execute(lhs_rule=["Milk", "Yogurt"], rhs_rule=["Eggs"], minsup=supp, minconf=conf)
    print_results(algo, supp, conf)

    print("As you can see, this rule doesn't hold well on this dataset. "
          "We have several clusters of violating transactions, suggesting a mismatch.")
    print("Observed exceptions suggest that the minimum support or confidence level needs "
          "to be changed, or that the entire rule needs to be revised.")
    print("Let's try modifying the rule. We will remove 'Yogurt' from the left side and re-validate "
          "the rule [Milk] -> [Eggs]. This represents a scenario where we weaken the "
          "rule to see if it better fits the data.\n")

    algo.execute(lhs_rule=["Milk"], rhs_rule=["Eggs"], minsup=supp, minconf=conf)
    print_results(algo, supp, conf)
    print("This highlights that our initial AR, with these parameters, might not be suitable "
          "for this dataset. We've chosen to modify the rule (by removing 'Yogurt'), "
          "which is just one approach. We could also decrease the minimum support value instead.\n\n")

def scenario_singular():
    table = pandas.read_csv(TABLE_SINGULAR, header=None, index_col=0)

    print("For the second example, let's take a dataset "
          'with the same receipts from the same supermarket, '
          'changing the input format to input_format="singular". This is a two-column format, '
          'where the first column contains the transaction number, and the second column specifies '
          'the item that belongs to that transaction.\n')
    supp, conf = 0.2, 0.6
    print('This way of representing the original data can sometimes be useful. Desbordante offers the same rule validation '
          'capabilities for this format and produces the same clusters of exceptions.\n')
    print('Lets re-run the first example over the same data represented in singular data format. We are checking the same rule [Bread] -> [Butter], '
          f'support = {supp} and confidence = {conf}:\n')
    print_table(table)
    algo = desbordante.ar_verification.algorithms.Default()
    algo.load_data(table=(TABLE_SINGULAR, ',', False), input_format='singular')
    algo.execute(lhs_rule=["Bread"], rhs_rule=["Butter"], minsup=supp, minconf=conf)
    print_results(algo, supp, conf)
    print("Everything is the same, except the transaction numbers, used in clusters (3 in singular vs 2 in tabular). "
          "This format offers explicit transaction IDs, therefore we use them instead of row numbers.")


if __name__ == '__main__':
    print("This example demonstrates how to validate Association Rules (AR) using the Desbordante library. "
          "Association rules discover relationships between itemsets in data (X -> Y). "
          "AR validation checks if rules hold by calculating Support (frequency of 'X union Y') "
          "and Confidence (how often Y appears in transactions with X), comparing them to thresholds. "
          "More details on association rules can be found "
          'in "Frequent Pattern Mining" by Charu C. Aggarwal.\n')
    print("Desbordante can be particularly helpful when association rules have already "
          "been mined, and changes occur in the data, potentially altering or invalidating existing rules. "
          "To understand what's happening with the data, Desbordante provides several types of clusters to categorize records, based on "
          "the completeness of the left and right-hand sides of the rule within each record: \n"
          "1) Transaction has a complete left side of the rule and an incomplete right side; \n"
          "2) Full left side, no right side; \n"
          "3) Incomplete left side, full right side; \n"
          "4) Incomplete left side, incomplete right side; \n"
          "5) Incomplete left side, no right side;\n"
          "Note that if either part of the rule consists of a single element, cluster types where the corresponding part is incomplete will be empty.\n")
    scenario_tabular()
    scenario_singular()
