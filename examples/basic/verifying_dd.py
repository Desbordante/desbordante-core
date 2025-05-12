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
        'This is an example of validating differential dependencies. The definition of differential dependencies is provided in the article by Song, Shaoxu, and Chen, Lei, "Differential Dependencies: Reasoning and Discovery," published in 2011 in the ACM Transactions on Database Systems (Vol. 36, No. 3).')
    print('An additional example of differential dependency mining is available in the Desbordante project.')
    print(
        'A differential dependency defines constraints on the differences between attribute values in tables.'
        'These dependencies are formalized through differential functions, which specify allowable distance ranges between attribute values.')
    print(
        '''For instance, consider a differential dependency expressed as date[0, 7] -> price[0, 250], consisting of two differential functions: date[0, 7] and price[0, 250].
Suppose this dependency is verified against a flight schedule table. If the dependency holds, it implies that for any two tuples where the difference in the date attribute does not exceed 7 days, the corresponding difference in the price attribute does not exceed 250 units.
Informally, this indicates that the price difference between any two flights scheduled within the same week remains within a 250-unit margin.'''
    )
    print("To further illustrate, we examine the dataset stores_dd.csv and attempt to validate a specified differential dependency.")
    data = pd.read_csv(table)
    print(f"{data}\n")


def print_first_dd_verified(algorithm):
    print(
        f"In order to finally understand the definition of difference dependency, let's look at the first example of differential dependency")
    print(f"It is dd: {COLOR_CODES["yellow"]}{dd1.__str__()}{COLOR_CODES["nocolor"]}\n")
    print_holds(algorithm.dd_holds())
    print(
        "It asserts that for any pair of rows with identical values in the product_name column, the difference in the stock_quantity attribute must be within the range [0, 20], and the difference in the price_per_unit attribute must be within [0, 60].")
    print(
        "Informally, this implies that for identical products across various stores, the stock quantity should not differ by more than 20 units, and the unit price should not differ by more than 60 units.")


def print_second_dd_verified(algorithm):
    print(#!!!
        f"Now, let`s check the dd: {COLOR_CODES["yellow"]}{dd2.__str__()}{COLOR_CODES["nocolor"]}, that means, what in one store stock quantity of goods has difference in range from 0 to 25\n")
    print_holds(algorithm.dd_holds())


def print_third_dd_verified(algorithm):
    print("The failure of the initial differential dependency is understandable, considering that stock quantities depend not only on the specific store but also on various operational factors, including product demand and store size.")
    print("To refine the dependency, an additional constraint on the product category is introduced.")
    print(f"Next dd, which we are going to check: {COLOR_CODES["yellow"]}{dd3.__str__()}{COLOR_CODES["nocolor"]}\n")
    print_holds(algorithm.dd_holds())
    print(
        '''The next differential dependency under examination states:

For any pair of rows with identical values in both the store_name and category columns, the difference in stock_quantity must not exceed 25 units.''')


def print_about_second_dd(algorithm):
    print(
        f'''Upon evaluation, it was observed that there are {algo.get_num_error_pairs()} pairs of rows where the difference in stock_quantity exceeds the specified threshold.''')
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
        print(f"This {COLOR_CODES["red"]}DD doesn`t hold{COLOR_CODES["nocolor"]}\n")


TABLE = '/home/yrii/spbu/desbordante-core-my/examples/datasets/stores_dd.csv'  # 'examples/datasets/stores_dd.csv'
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
