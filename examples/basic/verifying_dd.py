import csv

import desbordante
import pandas as pd

COLOR_CODES = {
    "green": "\033[1;32m",
    "red": "\u001b[1;31m",
    "yellow": "\033[1;33m",
    "cyan": "\033[1;36m",
    "nocolor": "\033[0m"
}


def print_intro(table: str) -> None:
    print(
        'This is an example of validation of differential dependencies.'
        '\n\nDefinitions of this dependency are provided in the article of Song, Shaoxu and Chen, Lei, '
        '\n"Differential dependencies: Reasoning and discovery", 2011, ACM Transactions on Database Systems Vol. 36, No. 3\n')
    print('There is also an example of mining differential dependencies in Desbordante.\n')
    print(
        'Differential dependency is a dependency, that define constraints on the differences of attributes in tables. '
        '\nDifferential Dependencies is defined by differential functions, that determine constraints on the distance between specific attributes.\n')
    print(
        'For example, a differential dependency of the form date[0, 7] -> price[0, 250] consists of two differential functions: date[0, 7] and price[0, 100]. '
        '\nSuppose this dependency is checked in a flight schedule. If such a differential dependency holds in the table, then for any two pairs of tuples where the distance between the values of the date attribute does not exceed 7 days, the difference in the price attribute does not exceed 100 units. '
        '\nInformally, this means that the price difference between two flights within the same week does not exceed 250 units.\n')
    print("Let's take the table stores_dd.csv and try to verify the dd in it.\n")
    data = pd.read_csv(table)
    print(f"{data}\n")


def print_first_dd_verified(algorithm):
    print(
        f"In order to finally understand the definition of difference dependence, let's look at the first example of differential dependency")
    print(f"It is dd: {COLOR_CODES["yellow"]}{dd1.__str__()}{COLOR_CODES["nocolor"]}\n")
    print_holds(algorithm.dd_holds())
    print(
        "It means, that for any pair rows with equal value from column \"product_name\", defined distance from 0 to 20 on column \"stock_quantity\" and from 0 to 60 on column \"price_per_unit\"\n")
    print("Informally, this means that in stores the difference in the quantity of goods available does not exceed 20 and the price of the same goods in stores does not exceed 60 units.")


def print_second_dd_verified(algorithm):
    print(
        f"Now, let`s check the dd: {COLOR_CODES["yellow"]}{dd2.__str__()}{COLOR_CODES["nocolor"]}, that means, what in one store stock quantity of goods has difference in range from 0 to 25\n")
    print_holds(algorithm.dd_holds())

def print_third_dd_verified(algorithm):
    print("The fact that the previous differential dependency was not holds in table is quite logical,\n"
          "since the stock quantity of goods depends not only on the specific store, but also on the selected product\n")
    print("Let's add a restriction to the product category and see if anything changes.\n")
    print(f"Next dd, which we are going to check: {COLOR_CODES["yellow"]}{dd3.__str__()}{COLOR_CODES["nocolor"]}\n")
    print_holds(algorithm.dd_holds())
    print(
        "It means, that for any pair rows with equal value from columns \"store_name\" and \"category\", defined distance from 0 to 25 on column \"stock_quantity\"\n")



def print_about_second_dd(algorithm):
    print(
        f"We have {algo.get_num_error_pairs()} pairs of rows, where distance in column \"stock_quantity\" is not holds.\n")
    highlights = algo.get_highlights()
    table_copy = table.copy()
    for highlight in highlights:
        first_error_row_index = highlight.pair_rows[0] + 1
        second_error_row_index = highlight.pair_rows[1] + 1
        print(f"{first_error_row_index}) ", end="")
        for i in range(0, 5):
            if i == highlight.attribute_index:
                print(f"{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ", end="")
            else:
                print(table_copy[first_error_row_index][i] + " ", end="")
        print()
        print(f"{second_error_row_index}) ", end="")
        for i in range(0, 5):
            if i == highlight.attribute_index:
                print(f"{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ", end="")
            else:
                print(table_copy[second_error_row_index][i] + " ", end="")
        print('\n')

    print(f"Error threshold: {algo.get_error()}")


def print_holds(result: bool) -> None:
    if result:
        print(f"This {COLOR_CODES["green"]}DD holds{COLOR_CODES["nocolor"]}\n")
    else:
        print(f"This {COLOR_CODES["red"]}DD Not holds{COLOR_CODES["nocolor"]}\n")


TABLE = 'examples/datasets/stores_dd.csv'
lhs1 = [desbordante.dd_verification.create_df("product_name", 0, 0)]
rhs1 = [desbordante.dd_verification.create_df("stock_quantity", 0, 20),
        desbordante.dd_verification.create_df("price_per_unit", 0, 60)]
dd1 = desbordante.dd_verification.create_dd(lhs1, rhs1)
lhs2 = [desbordante.dd_verification.create_df("store_name", 0, 0)]
rhs2 = [desbordante.dd_verification.create_df("stock_quantity", 0, 25)]
dd2 = desbordante.dd_verification.create_dd(lhs2, rhs2)
lhs3 = [desbordante.dd_verification.create_df("store_name", 0, 0),
        desbordante.dd_verification.create_df("category", 0, 0)]
rhs3 = [desbordante.dd_verification.create_df("stock_quantity", 0, 25)]
dd3 = desbordante.dd_verification.create_dd(lhs3, rhs3)
algo = desbordante.dd_verification.algorithms.DDVerifier()
algo.load_data(table=(TABLE, ',', True))
algo.execute(dds=dd1)
with open(TABLE, newline='') as csvtable:
    table = list(csv.reader(csvtable))
print_intro(TABLE)
print("-" * 100)
print("Example #1\n")
print_first_dd_verified(algo)
print("-" * 100)
print("Example #2\n")
algo.execute(dds=dd2)
print_second_dd_verified(algo)
print_about_second_dd(algo)
print("-" * 100)
print("Example #3\n")
algo.execute(dds=dd3)
print_third_dd_verified(algo)
