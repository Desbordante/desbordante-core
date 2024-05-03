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

def print_table(table, title = None):
    if title is not None:
        print(title)

    print(tabulate(table, tablefmt='pipe', stralign='center'))


def print_ars(ars):
    print('Total count of ARs:', len(ars))
    print('The first 10 ARs:')
    for ar in ars[:10]:
        if ar.confidence > 0.9:
            print(COLOR_CODES['bold_green'], end='')
        elif ar.confidence > 0.3:
            print(COLOR_CODES['bold_yellow'], end='')
        else:
            print(COLOR_CODES['bold_red'], end='')

        print('{:1.2f}'.format(ar.confidence), 
              COLOR_CODES['default'], end='\t')
        print(ar.left, '->', ar.right, )


def print_itemnames(itemnames):
    print(f'{COLOR_CODES["blue_bg"]}Total number of items:'
          f'{COLOR_CODES["default_bg"]} {len(itemnames)}')
    print(*itemnames, sep='\n')


def scenario_tabular():
    algo = desbordante.ar.algorithms.Default()
    algo.load_data(table=(TABLE_TABULAR, ',', True), input_format='tabular')
    algo.execute(minconf=1)
    table = pandas.read_csv(TABLE_TABULAR, header=None)

    print("As the first example, let's look at the dataset containing receipts "
          'from some supermarket using input_format="tabular". In this '
          'format, each table row lists all items participating in the same '
          'transaction. Note that, in this table, '
          'some rows may be shorter than others.\n')
    print_table(table)
    print("\nLet's see the first 10 association rules (ARs) that are present "
          'in the dataset with minconf=1. As no minsup is specified, '
          'the default value of minsup=0 is used.\n')
    print_ars(algo.get_ars())
    print("\n['Eggs'] -> ['Milk'] with confidence 1 means that whenever eggs "
          'are found in the receipt, milk will '
          f'{COLOR_CODES["green"]}always{COLOR_CODES["default"]} '
          'be present as well. The same holds true for all other rules with '
          f'{COLOR_CODES["bold_green"]}confidence 1{COLOR_CODES["default"]}.')

    print("\n\nNow, let's examine the same dataset with "
          f'{COLOR_CODES["yellow"]}minconf=0.7{COLOR_CODES["default"]}.')
    algo.execute(minconf=0.7)
    print_ars(algo.get_ars())
    print("\n['Milk'] -> ['Eggs'] with confidence 0.75 means that when milk "
          'is found in the receipt, the chance of eggs being '
          'present amounts to 75%. So, customers are '
          f'{COLOR_CODES["bold_yellow"]}likely{COLOR_CODES["default"]} '
          'to buy eggs with milk.')

    print('\nLet us turn to the next issue. You can observe that '
          'there are a lot of association rules found in this '
          'small dataset. This happens because we did not set up the so '
          f'called {COLOR_CODES["green"]}support{COLOR_CODES["default"]} '
          'value. This value is a a float between 0 and 1 that specifies '
          'how frequently the itemset should appear in the dataset. '
          'In this particular case, the frequency is defined '
          'as the ration between the number of transactions containing '
          'A and B, and the total number of transactions in a dataset. '
          'Since the default support value is 0, the system discovers '
          'all association rules, even those that only occur once '
          "in the dataset. Now, let's see the results with "
          f'{COLOR_CODES["yellow"]}minsup=0.5{COLOR_CODES["default"]} and '
          f'{COLOR_CODES["yellow"]}minconf=0.5{COLOR_CODES["default"]}.\n')
    algo.execute(minsup=0.5, minconf=0.5)
    print_ars(algo.get_ars())
    print('\nNow you can see that the number of association rules have decreased '
          'significantly. This happened due to minsup being set to 0.5. '
          'Unfortunately, if you want to know what the support value is for a '
          'particular association rule in a dataset, '
          "you can't get it with Desbordante.\n"
          '\nA typical approach to controlling the algorithm is to employ '
          f'{COLOR_CODES["bold_yellow"]}"usefulness"{COLOR_CODES["default"]}, '
          'which is defined as confidence * support. '
          'In the last example, we set up min '
          '"usefulness" = 0.5 * 0.5 = 0.25. \n\nNow, let\'s try with '
          f'{COLOR_CODES["green"]}minsup=0.7{COLOR_CODES["default"]}, '
          f'{COLOR_CODES["green"]}minconf=0.7{COLOR_CODES["default"]} and '
          f'{COLOR_CODES["bold_yellow"]}"usefulness"=0.49{COLOR_CODES["default"]}.\n')
    algo.execute(minsup=0.7, minconf=0.7)
    print_ars(algo.get_ars())
    print('\nSo, now the total number of returned association rules '
          'is only four. We reduced the amount of "noisy" information '
          'in our output. You are free to play with these parameters '
          'to see how it changes things. Eventually, you will '
          'find out what fits your dataset and your task best.')


def scenario_singular():
    algo = desbordante.ar.algorithms.Default()
    algo.load_data(table=(TABLE_SINGULAR, ',', True), input_format='tabular')
    algo.execute()
    table = pandas.read_csv(TABLE_SINGULAR, header=None, index_col=0)

    print("\nFor the second example, let's take a dataset "
          'with the same receipts from the same supermarket, '
          'changing the input format to input_format="singular". This is a '
          'two-column format, where the first column is the order of the items, '
          'and the second column is the item that belongs to that order.\n')
    print_table(table)
    print('\nThis format is just a different table representation. Desbordante '
          'is perfectly capable of rules discovery in such tables, and the '
          'discovered objects are represented by the same '
          'type of objects as in the previous examples ' 
          '(i.e., they have the same methods and fields).\n\n')
    print('Next, we will show you how to list all '
          'of the unique items in the dataset.\n')
    print(f'In order to do this, you can use {COLOR_CODES["bold_blue"]}'
          f'get_itemnames{COLOR_CODES["default"]} method:\n')
    print_itemnames(algo.get_itemnames())
    print('\nNow you have all of the items used in this dataset, '
          'allowing you to filter them and then print individual entries.\n')
    print_itemnames(list(filter(lambda item: not item.isdigit(),  algo.get_itemnames())))
   

if __name__ == '__main__':
    scenario_tabular()
    scenario_singular()
