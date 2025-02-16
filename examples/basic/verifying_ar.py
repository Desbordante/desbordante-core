import desbordante
import pandas
from tabulate import tabulate

TABLE_TABULAR = 'examples/datasets/rules_book_rows.csv'
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


def print_results(ar_verifier, supp, conf):
    if ar_verifier.ar_holds():
        print(f'AR holds: {COLOR_CODES["bold_green"]}{ar_verifier.ar_holds()}{COLOR_CODES["default"]}')
    else:
        print(f'AR holds: {COLOR_CODES["bold_red"]}{ar_verifier.ar_holds()}{COLOR_CODES["default"]}')
        print(f'Total number of transactions violating AR: {ar_verifier.get_num_transactions_violating_ar()}')
        print(f'Number of clusters violating AR: {ar_verifier.get_num_clusters_violating_ar()}')
        print('Clusters violating AR:')
        clusters_violating_ar = ar_verifier.get_clusters_violating_ar()
        for priority, cluster in clusters_violating_ar.items():
            print(f'{priority} : {cluster}')
        actual_supp, actual_conf = ar_verifier.get_real_support(), ar_verifier.get_real_confidence()
        print('actual confidence: ', end='')
        if actual_conf >= conf:
            print(COLOR_CODES['bold_green'], end='')
        elif actual_conf + 0.5 > conf:
            print(COLOR_CODES['bold_yellow'], end='')
        else:
            print(COLOR_CODES['bold_red'], end='')

        print('{:1.2f}'.format(actual_conf),
              COLOR_CODES['default'], end='\t')
        print('actual support: ', end='')

        if actual_supp >= supp:
            print(COLOR_CODES['bold_green'], end='')
        elif actual_supp + 0.5 > supp:
            print(COLOR_CODES['bold_yellow'], end='')
        else:
            print(COLOR_CODES['bold_red'], end='')

        print('{:1.2f}'.format(actual_supp),
              COLOR_CODES['default'], end='\t')
    print()

# Loading input data
def scenario_tabular():
    table = pandas.read_csv(TABLE_TABULAR, header=None)
    print("As the first example, let's look at the dataset containing receipts "
          'from some supermarket using input_format="tabular". In this '
          'format, each table row lists all items participating in the same '
          'transaction. Note that, in this table, '
          'some rows may be shorter than others.\n')
    print_table(table)
    algo = desbordante.ar_verification.algorithms.Default()
    algo.load_data(table=(TABLE_TABULAR, ',', False), input_format='tabular')
    supp, conf = 0.6, 0.4
    print(f"let's check if the rule [Bread] -> [Butter] holds for the given confidence {conf} and support {supp}:\n")
    algo.execute(arule_left=["Bread"], arule_right=["Butter"], minsup=supp, mniconf=conf)
    print_results(algo, supp, conf)
    print("\nYou can see that the actual values of support and confidence "
          "are different from those presented. Let's look at the clusters "
          "that violate the rule: only one transaction (with index 2 in table) was found that is a "
          "potential error: it contains the entire left part of the association "
          "rule, but does not have the right one:\n")
    print(table.iloc[2].to_dict())
    print()
    print("This can signal two cases: \n"
          "-First, and most likely, the rule simply does not hold. We expect too "
          "high a support and confidence values.\n"
          "-Second, the data was corrupted and the record of the purchase of the "
          "product 'Butter' was missing from the second transaction. Since this dataset "
          "is quite small, the probability of such an outcome is also small.\n")

    algo = desbordante.ar_verification.algorithms.Default()
    algo.load_data(table=(TABLE_TABULAR, ',', False), input_format='tabular')
    supp, conf = 0.45, 0.75
    table = pandas.read_csv(TABLE_TABULAR, header=None)
    print("Let's now look at a rule whose left side consists of several elements. "
          f"Rule [Milk, Yogurt] -> [Eggs] with support = {supp}, confidence = {conf}")
    print_table(table)
    algo.execute(arule_left=["Milk", "Yogurt"], arule_right=["Eggs"], minsup=supp, mniconf=conf)
    print_results(algo, supp, conf)
    print("\nAs you can see, we now have several types of clusters with violating transactions. "
          "The cluster priority depends first on the completeness of the left part, and "
          "then on the right part.")
    print('from the algorithm result we can draw several conclusions:\n'
          '-In transactions 2 or 4 the records of the purchase of the product "Eggs"\n'
          'or "Yogurt" were lost respectively.\n'
          '-We expect too high confidence value.\n'
          '-It is worth weakening the rule by removing the element "Yogurt" from the left part of the rule.')
if __name__ == '__main__':
    scenario_tabular()
