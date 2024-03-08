import desbordante
import csv
from tabulate import tabulate

COLOR_CODES = {
    "green": "\033[1;32m",
    "red": "\u001b[1;31m",
    "yellow": "\033[1;33m",
    "cyan": "\033[1;36m",
    "nocolor": "\033[0m"
}


def print_table(table, indices=None):
    if indices is None:
        print(tabulate(table, headers='firstrow', tablefmt='psql', showindex=True))
    else:
        print(tabulate(table, headers='firstrow', tablefmt='psql', showindex=indices))


def color_row(row, color_map):
    row_copy = row.copy()
    for index,color in color_map.items():
        row_copy[index] = f'{COLOR_CODES[color]}{row_copy[index]}{COLOR_CODES["nocolor"]}'
    return row_copy


def print_introduction():
    print("Consider the table containing some information about flights:")
    print_table(table)
    print()


def print_dependencies(dds):
    print("Here are the differential dependencies (DDs) that were discovered from this table by the SPLIT algorithm:")
    print()
    index = 1
    for dd in dds:
        print(f"{index}) ", dd)
        index += 1
    print()


def print_first_explanation(dd):
    print(f"The DD \"{dd}\" means the following.")
    print()
    print("For any two tuples of the table if")
    print("a) the distance between them on the column \"Departure\" is between 0 and 0 (i.e. they are equal), and")
    print("b) the distance between them on the column \"Arrival\" is between 0 and 0 (i.e. they are equal),")
    print("then the distance between them on the column \"Distance\" is between 0 and 50.")
    print()
    print("The only tuple pair that satisfies both of the constraints on the left-hand side is (0,2):")
    table_copy = table.copy()
    table_copy[1] = color_row(table_copy[1], {2: "green", 3: "green"})
    table_copy[3] = color_row(table_copy[3], {2: "green", 3: "green"})
    print_table(table_copy)
    print()
    print("Now let's consider the values of this tuple pair on the column \"Distance\":")
    table_copy = table.copy()
    table_copy[1] = color_row(table_copy[1], {2: "green", 3: "green", 4: "red"})
    table_copy[3] = color_row(table_copy[3], {2: "green", 3: "green", 4: "red"})
    print_table(table_copy)
    print("We can notice that the distance is between 0 and 50. Therefore, the DD")
    print(f"\"{dd}\" holds in the table.")
    print()


def print_second_explanation(dd):
    print(f"Now let's move to the second DD: \"{dd}\". This DD means the following: for any")
    print("pair of tuples if the distance between them on the column \"Distance\" is between 0 and 50, then the distance on")
    print("the column \"Duration\" is between 0 and 15. In other words, if two flights have similar distances, then they")
    print("last for a similar time. As can be seen from the table, almost all flights have similar distances which differ")
    print("by less than 50 kilometers.")
    print()
    print("Next, for all flights from 0 to 9 their durations are between 58 and 73 minutes, so the difference is less or")
    print("equal to 15 minutes. Therefore, the second DD also holds in the table.")
    print()


def print_third_explanation(dd):
    print(f"Now consider the third DD: \"{dd}\". It means that for any two")
    print("tuples from the table if")
    print("a) the distance between them on the column \"Departure\" is between 0 and 3, and")
    print("b) on the column \"Arrival\" the distance is between 0 and 3,")
    print("then the distance on the column \"Duration\" is between 0 and 15.")
    print()
    print("The distance between two strings is their edit distance (the number of characters that need to be substituted,")
    print("deleted or inserted in order to turn the first string into the second).")
    print()
    print("The distance constraint \"Departure [0, 3]\" means that we consider only those tuple pairs whose values on the")
    print("column \"Departure\" are close enough. In this case we aim to consider the airports located in the same cities.")
    print("For example, tuple pairs (0,1) and (3,4) are satisfying this constraint.")
    print()
    print("Tuples 0 and 1 have the same departure airport:")
    print_table([table[0], color_row(table[1], {2: "green"}), color_row(table[2], {2: "green"})], [0,1])
    print()
    print("Tuples 3 and 4 have the same city but different airport codes:")
    print_table([table[0], color_row(table[4], {2: "green"}), color_row(table[5], {2: "green"})], [3,4])
    print()
    print("For the distance constraint \"Arrival [0, 3]\" the situation is similar.")
    print("Here are the tuple pairs that satisfy both of the constraints on the left-hand side:")
    table_copy = table.copy()
    table_copy[1] = color_row(table_copy[1], {2: "green", 3: "green"})
    table_copy[2] = color_row(table_copy[2], {2: "green", 3: "green"})
    table_copy[3] = color_row(table_copy[3], {2: "green", 3: "green"})
    table_copy[7] = color_row(table_copy[3], {2: "green", 3: "green"})
    table_copy[4] = color_row(table_copy[4], {2: "yellow", 3: "yellow"})
    table_copy[5] = color_row(table_copy[5], {2: "yellow", 3: "yellow"})
    table_copy[6] = color_row(table_copy[6], {2: "yellow", 3: "yellow"})
    table_copy[8] = color_row(table_copy[8], {2: "cyan", 3: "cyan"})
    table_copy[9] = color_row(table_copy[9], {2: "cyan", 3: "cyan"})
    print_table(table_copy)
    print()
    print("Now let's consider the values of these tuple pairs on the column \"Duration\":")
    table_copy = table.copy()
    table_copy[1] = color_row(table_copy[1], {2: "green", 3: "green", 5: "green"})
    table_copy[2] = color_row(table_copy[2], {2: "green", 3: "green", 5: "green"})
    table_copy[3] = color_row(table_copy[3], {2: "green", 3: "green", 5: "green"})
    table_copy[7] = color_row(table_copy[3], {2: "green", 3: "green", 5: "green"})
    table_copy[4] = color_row(table_copy[4], {2: "yellow", 3: "yellow", 5: "yellow"})
    table_copy[5] = color_row(table_copy[5], {2: "yellow", 3: "yellow", 5: "yellow"})
    table_copy[6] = color_row(table_copy[6], {2: "yellow", 3: "yellow", 5: "yellow"})
    table_copy[8] = color_row(table_copy[8], {2: "cyan", 3: "cyan", 5: "cyan"})
    table_copy[9] = color_row(table_copy[9], {2: "cyan", 3: "cyan", 5: "cyan"})
    print_table(table_copy)
    print()
    print("It can easily be seen that for every highlighted tuple pair their duration differs by up to 15 minutes. Therefore,")
    print(f"the DD \"{dd}\" holds in the table.")
    print()


def print_information_about_difference_table(dds):
    print("The most important parameter of the SPLIT algorithm for DD discovery is the difference table. Here is the")
    print("difference table that was used in this example:")
    print()
    print_table(dif_table)
    print()
    print("The difference table defines the search space for DDs. That means, the algorithm searches only for DDs constructed")
    print("from distance constraints stated in the difference table. Therefore, as you can see from the discovered DDs, all")
    print("of the distance constraints that were used there are stated in the difference table.")
    print()
    print("The number of constraints for each column can be different. The difference table can be accepted by the algorithm")
    print("only in the format stated above. Note that different difference tables processed by the algorithm result in")
    print("different sections of the search space and lead to different results.")
    print()
    print("For example, consider another difference table:")
    print_table(dif_table1)
    print()
    print("The result for the algorithm executed with this difference table is following:")
    print()
    index = 1
    for dd in dds:
        print(f"{index}) ", dd)
        index += 1
    print()
    print("Note that the distance constraint in the third DD has been changed from \"Arrival [0, 3]\" to \"Arrival [0, 0]\".")
    print("That has happened because the constraint \"Arrival [0, 3]\" is no more in the search space.")


TABLE = "examples/datasets/flights_dd.csv"
DIF_TABLE = "examples/datasets/flights_dd_dif_table.csv"
DIF_TABLE1 = "examples/datasets/flights_dd_dif_table1.csv"

algo = desbordante.dd.algorithms.Split()
algo.load_data(table=(TABLE, ',', True))
algo.execute(difference_table=(DIF_TABLE, ',', True))
result = algo.get_dds()
algo.execute(difference_table=(DIF_TABLE1, ',', True))
result1 = algo.get_dds()

with open(TABLE, newline='') as csvtable:
        table = list(csv.reader(csvtable))
with open(DIF_TABLE, newline='') as csvtable:
        dif_table = list(csv.reader(csvtable))
with open(DIF_TABLE1, newline='') as csvtable:
        dif_table1 = list(csv.reader(csvtable))

print_introduction()
print_dependencies(result)
print_first_explanation(result[0])
print_second_explanation(result[1])
print_third_explanation(result[2])
print_information_about_difference_table(result1)
