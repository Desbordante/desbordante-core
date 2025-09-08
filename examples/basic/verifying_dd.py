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
        f'''This is an example of validating differential dependencies.

Differential dependencies were introduced by Song, Shaoxu,
and Chen, Lei in their 2011 article, "Differential
Dependencies: Reasoning and Discovery," published in ACM
Transactions on Database Systems (Vol. 36, No. 3).

A differential dependency (DD) defines constraints on the
differences between attribute values within a table.
These dependencies are formalized using differential
functions, which specify permissible distance ranges
between attribute values.

For instance, consider the following differential
dependency:

flight_id[0,0]; date[0, 7] -> price[0, 250].

This expression is composed of three differential functions:
flight_id[0,0], date[0, 7], and price[0, 250]. If this
dependency is applied to a flight schedule table, it
implies that for any two tuples where the difference
for identical flights (sharing the same id) in the date
attribute is no more than 7 days, the corresponding
difference in the price attribute must not exceed 250 units.
In simpler terms, this means that the price difference
between any two identical flights scheduled within the
same week will be within a 250-unit margin.

Flight schedule table:

{pd.read_csv(FLIGHT_TABLE)}

It can be observed that this dependency holds true for the
flights table.

To illustrate this further, we will examine the
stores_dd.csv dataset and validate a specified
differential dependency.

It is worth noting that this validator implements
standard distance metrics for numerical data types
and dates, as well as the Levenshtein distance for
strings.

Finally, an additional example of differential dependency
mining, using the Split algorithm, is also available in
the Desbordante project.\n''')
    data = pd.read_csv(table)
    print(f"{data}\n")


def print_first_dd_verified(algorithm):
    print(
        f'''To better understand the differential dependency concept,
let's examine a practical example.

Consider the following DD:

{COLOR_CODES["yellow"]}{dd1.__str__()}{COLOR_CODES["nocolor"]}
''')
    print_holds(algorithm.dd_holds())
    print(
        '''This dependency requires that for any two records
with the same product_name, the difference in
their stock_quantity must not exceed 20, and
the difference in price_per_unit must not
exceed 60.

In other words, for the same product sold
across different stores, stock levels cannot
vary by more than 20 units, and the price
cannot vary by more than 60 units.''')


def print_second_dd_verified(algorithm):
    print(
        f'''Now, let`s check the DD:

{COLOR_CODES["yellow"]}{dd2.__str__()}{COLOR_CODES["nocolor"]}

This means that for a single product, the
difference in stock quantity between any
two stores cannot exceed 25 units.
''')
    print_holds(algorithm.dd_holds())


def print_third_dd_verified(algorithm):
    print('''Our previous dependency failed because it
didn't account for key operational factors.
For instance, stock levels are influenced
by product demand and store size, not just
the store location.

To address this, we refine the dependency
by adding a constraint on the product_category.''')
    print(f'Next DD, which we are going to check: \n\n{COLOR_CODES["yellow"]}{dd3.__str__()}{COLOR_CODES["nocolor"]}\n')
    print_holds(algorithm.dd_holds())
    print(
        '''This differential dependency states:

For tuples with the same store_name and category,
the stock_quantity must not differ by more than
25 units.''')


def print_about_second_dd(algorithm):
    print('''Desbordante can automatically detect pairs of
violating tuples. Let’s do it.
''')
    print(
        f'''Desbordante returned {algo.get_num_error_pairs()} pairs of records that
violate the stock_quantity threshold.\n''')
    print('''The error threshold of a differential
dependency is the ratio of record pairs
that satisfy the left-hand side (LHS) but
violate the right-hand side (RHS), to the
total number of record pairs that satisfy
the LHS.
''')
    print(f'In our example error threshold is: {COLOR_CODES["red"]}{algo.get_error()}{COLOR_CODES["nocolor"]}\n\nNow, let us look at the pairs of violating tuples:\n')
    highlights = algo.get_highlights()
    table_copy = table.copy()
    for highlight in highlights:
        first_error_row_index = highlight.pair_rows[0] + 1
        second_error_row_index = highlight.pair_rows[1] + 1
        print(f"{first_error_row_index}) ", end="")
        for i in range(0, 5):
            if i == 0:
                print(f'{COLOR_CODES["green"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            elif i == highlight.attribute_index:
                if i == 4:
                    print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                else:
                    print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            else:
                if i == 4:
                    print(table_copy[first_error_row_index][i], end="")
                else:
                    print(table_copy[first_error_row_index][i] + " ", end="")
        print()
        print(f"{second_error_row_index}) ", end="")
        for i in range(0, 5):
            if i == 0:
                print(f'{COLOR_CODES["green"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            elif i == highlight.attribute_index:
                if i == 4:
                    print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                else:
                    print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            else:
                if i == 4:
                    print(table_copy[second_error_row_index][i], end="")
                else:
                    print(table_copy[second_error_row_index][i] + " ", end="")
        print('\n')
    print('''Clearly, this DD has no practical
significance, however, it remains useful for
demonstration purposes.''')


def print_about_fourth_dd():
    data = pd.read_csv(GRADES_TABLE)
    print('''Validation of Differential Dependencies can
also be utilized for mitigating data inaccuracies.

Consider the grades_dd.py table.
''')
    print(data)
    print(f'''
We will check the DD:

{COLOR_CODES["yellow"]}{dd4.__str__()}{COLOR_CODES["nocolor"]}''')
    algorithm = desbordante.dd_verification.algorithms.DDVerifier()
    activity = (GRADES_TABLE, ',', True)
    algorithm.load_data(table=activity)
    algorithm.execute(dd=dd4)
    print()
    print_holds(algorithm.dd_holds())
    highlights = algorithm.get_highlights()
    table_copy: list[list[str]]
    with open(GRADES_TABLE, newline='') as act_table:
        act = list(csv.reader(act_table))
        table_copy = act.copy()
    for highlight in highlights:
        first_error_row_index = highlight.pair_rows[0] + 1
        second_error_row_index = highlight.pair_rows[1] + 1
        print(f"{first_error_row_index}) ", end="")
        for i in range(0, 5):
            if i == 0:
                print(f'{COLOR_CODES["green"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            elif i == highlight.attribute_index:
                if i == 4:
                    print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                else:
                    print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            else:
                if i == 4:
                    print(table_copy[first_error_row_index][i], end="")
                else:
                    print(table_copy[first_error_row_index][i] + " ", end="")
        print()
        print(f"{second_error_row_index}) ", end="")
        for i in range(0, 5):
            if i == 0:
                print(f'{COLOR_CODES["green"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            elif i == highlight.attribute_index:
                if i == 4:
                    print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                else:
                    print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
            else:
                if i == 4:
                    print(table_copy[second_error_row_index][i], end="")
                else:
                    print(table_copy[second_error_row_index][i] + " ", end="")
        print('\n')
    print(f'Error threshold: {COLOR_CODES["red"]}{algorithm.get_error()}{COLOR_CODES["nocolor"]}\n')
    print('''We have two pairs of rows that do not conform to
the constraints imposed by the DD. Let's rectify
this data error by changing "Akice" to "Alice"
in the first row of the "student_name" column
and then re-evaluate the DD's over this table.
''')
    data = pd.read_csv(GRADES_TABLE2)
    print(data)
    algorithm2 = desbordante.dd_verification.algorithms.DDVerifier()
    algorithm2.load_data(table =(GRADES_TABLE2, ',', True))
    algorithm2.execute(dd=dd4)
    print()
    if not algorithm2.dd_holds():
        print(f'''After correcting the error, the error threshold
dropped to {COLOR_CODES["red"]}{algorithm2.get_error()}{COLOR_CODES["nocolor"]}''')
    print("""
A potential error may also exist in the left-hand
side of the DD. For instance, in rows 3, 4 and 5
of the table, we have three entries with an identical
student_id but a different student_name. Let's
correct this error and observe the subsequent changes.
""")
    data = pd.read_csv(GRADES_TABLE3)
    print(data)
    algorithm3 = desbordante.dd_verification.algorithms.DDVerifier()
    algorithm3.load_data(table=(GRADES_TABLE3, ',', True))
    algorithm3.execute(dd=dd4)
    print()
    if algorithm3.dd_holds():
        print(f'''After correcting the error, the error threshold
dropped to {COLOR_CODES["green"]}{algorithm3.get_error()}{COLOR_CODES["nocolor"]} and the {COLOR_CODES["green"]}DD holds.{COLOR_CODES["nocolor"]}''')







def print_holds(result: bool) -> None:
    if result:
        print(f'This {COLOR_CODES["green"]}DD holds.{COLOR_CODES["nocolor"]}\n')
    else:
        print(f'This {COLOR_CODES["red"]}DD doesn`t hold.{COLOR_CODES["nocolor"]}\n')


TABLE = 'examples/datasets/stores_dd.csv'  # 'examples/datasets/stores_dd.csv'
FLIGHT_TABLE = 'examples/datasets/flights_dd_verifier_example.csv' # 'examples/datasets/flights_dd_verifier_example.csv'
GRADES_TABLE = 'examples/datasets/grades_dd.csv'
GRADES_TABLE2 = 'examples/datasets/grades_dd2.csv'
GRADES_TABLE3 = 'examples/datasets/grades_dd3.csv'
lhs1 = [desbordante.dd_verification.DF("product_name", 0, 0)]
rhs1 = [desbordante.dd_verification.DF("stock_quantity", 0, 20),
        desbordante.dd_verification.DF("price_per_unit", 0, 60)]
dd1 = desbordante.dd.DD(lhs1, rhs1)
lhs2 = [desbordante.dd_verification.DF("store_name", 0, 0)]
rhs2 = [desbordante.dd_verification.DF("stock_quantity", 0, 25)]
dd2 = desbordante.dd.DD(lhs2, rhs2)
lhs3 = [desbordante.dd_verification.DF("store_name", 0, 0),
        desbordante.dd_verification.DF("category", 0, 0)]
rhs3 = [desbordante.dd_verification.DF("stock_quantity", 0, 25)]
dd3 = desbordante.dd.DD(lhs3, rhs3)
algo = desbordante.dd_verification.algorithms.DDVerifier()
algo.load_data(table=(TABLE, ',', True))
algo.execute(dd=dd1)
lhs4 = [desbordante.dd_verification.DF("student_id", 0, 0)]
rhs4 = [desbordante.dd_verification.DF("student_name", 0, 0)]
dd4 = desbordante.dd.DD(lhs4, rhs4)
with open(TABLE, newline='') as csvtable:
    table = list(csv.reader(csvtable))
print_intro(TABLE)
print("-" * 100)
print("Example #1\n")
print_first_dd_verified(algo)
print("-" * 100)
print("Example #2\n")
algo.execute(dd=dd2)
print_second_dd_verified(algo)
print_about_second_dd(algo)
print("-" * 100)
print("Example #3\n")
algo.execute(dd=dd3)
print_third_dd_verified(algo)
print("-" * 100)
print("Example #4\n")
print_about_fourth_dd()
