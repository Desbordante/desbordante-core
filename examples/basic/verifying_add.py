import pandas as pd
import desbordante
import csv

COLOR_CODES = {
    "green": "\033[1;32m",
    "red": "\u001b[1;31m",
    "yellow": "\033[1;33m",
    "cyan": "\033[1;36m",
    "nocolor": "\033[0m"
}


def print_holds(algo, table, is_ADD=True) -> None:
    if algo.dd_holds():
        print(f'''This {COLOR_CODES["green"]}{'ADD' if is_ADD else 'DD'} holds{COLOR_CODES["nocolor"]}, error threshold is {COLOR_CODES["green"]}{algo.get_error()}{COLOR_CODES["nocolor"]}.\n''')
    else:
        print(f'''This {COLOR_CODES["red"]}{'ADD' if is_ADD else 'DD'} doesn`t hold{COLOR_CODES["nocolor"]}, error threshold is {COLOR_CODES["red"]}{algo.get_error()}{COLOR_CODES["nocolor"]}.\n''')

        print(f"{COLOR_CODES['cyan']}Violations found:{COLOR_CODES['nocolor']}")
        highlights = algo.get_highlights()
        with open(table, newline='') as f:
            table = list(csv.reader(f))
            table_copy = table.copy()
        for highlight in highlights:
            first_error_row_index = highlight.pair_rows[0] + 1
            second_error_row_index = highlight.pair_rows[1] + 1
            
            print(f"{first_error_row_index}) ", end="")
            for i in range(0, 4):
                if i == 1:
                    print(f'{COLOR_CODES["green"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                elif i == highlight.attribute_index:
                    print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                else:
                    print(table_copy[first_error_row_index][i] + " ", end="")
            print()
            print(f"{second_error_row_index}) ", end="")
            for i in range(0, 4):
                if i == 1:
                    print(f'{COLOR_CODES["green"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                elif i == highlight.attribute_index:
                    print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                else:
                    print(f'{table_copy[second_error_row_index][i]} ', end="")
            print('\n')

print(f'''This example demonstrates validation of approximate differential dependencies.
      
Approximate Differential Dependencies were introduced by Jixue Liu, Selasi Kwashie, 
Jiuyong Li, Feiyue Ye and Millist Vincent in their 2013 article, "Discovery of
Approximate Differential Dependencies". URL: https://arxiv.org/abs/1309.3733
      
A differential dependency (DD) defines constraints on the differences between
attribute values within a table. These dependencies are formalized using 
differential functions, which specify permissible distance ranges between 
attribute values. 
      
An approximate differential dependency (ADD) with satisfaction threshold eps 
is a DD where for all tuple pairs satisfying the left-hand side, at least eps 
fraction of them with the lowest distance satisfy the right-hand side. 
This change allows focusing on meaningful patterns while filtering out 
outliers and data errors.
''')
print('-' * 49)

print('''Example #1 

First, let's look at ADD validation compared to DD 
to better understand the definition.
We'll use an employee salary dataset to demonstrate.
''')
TABLE0 = 'examples/datasets/add_example.csv'
df0 = pd.read_csv(TABLE0)

print(df0, end='\n\n')

lhs = [desbordante.dd_verification.DF("education_level", 0, 0)]
rhs = [desbordante.dd_verification.DF("salary", 0, 15000)]
dd = desbordante.dd.DD(lhs, rhs)

print(f'''
Consider the following DD:

{COLOR_CODES["yellow"]}{dd.__str__()}{COLOR_CODES["nocolor"]}

This dependency means that for people with the same education level
salary differences should not exceed 15000.

Let's check whether this DD holds: 
''')
algoDD = desbordante.dd_verification.algorithms.Default()
algoDD.load_data(table=(TABLE0, ',', True))
algoDD.execute(dd=dd)
print_holds(algoDD, TABLE0, is_ADD=False)

print('''Now let's test the same dependency as an ADD with satisfaction 
threshold eps = 0.3:
''')

algoADD = desbordante.add_verification.algorithms.Default()
algoADD.load_data(table=(TABLE0, ',', True))
algoADD.execute(dd=dd, satisfaction_threshold=0.3)
print_holds(algoADD, TABLE0)

print('''Note that the error thresholds for DD and ADD are different. For DD,
the error threshold is determined by the proportion of tuple pairs that
violate the rhs among those that satisfy the lhs. Meanwhile, for ADD,
the error threshold is defined as 1 minus the proportion of tuple pairs
with the minimum distance that satisfy the rhs out of all tuple pairs
satisfying the lhs.

For the given dependency, the minimum distance for tuple pairs satisfying 
the lhs on the salary attribute is 2000. The fraction of these tuple pairs is
0.375, and all of them satisfy the rhs. Consequently, the ADD will hold as 
long as the satisfaction threshold is less than or equal to this fraction.

Let's change the satisfaction threshold for this ADD to 0.4 and verify that 
it does not hold.
''')

algoADD.execute(dd=dd, satisfaction_threshold=0.38)
if not algoADD.dd_holds():
        print(f'''This {COLOR_CODES["red"]}{'ADD'} doesn`t hold{COLOR_CODES["nocolor"]}, error threshold is {COLOR_CODES["red"]}{algoADD.get_error()}{COLOR_CODES["nocolor"]}.\n''')

lhs1 = [desbordante.dd_verification.DF("education_level", 0, 0)]
rhs1 = [desbordante.dd_verification.DF("salary", 0, 200000)]
dd1 = desbordante.dd.DD(lhs1, rhs1)

print('-' * 49)
print(f'''Example #2 

Because of the different meaning of the error threshold, it is possible for 
a DD to be satisfied while the ADD is not.

Let's consider the dependency:

{COLOR_CODES["yellow"]}{dd1.__str__()}{COLOR_CODES["nocolor"]}
''')

algoDD.execute(dd=dd1)
algoADD.execute(dd=dd1, satisfaction_threshold=0.4)

print_holds(algoDD, TABLE0, False)
print('''Also, consider an ADD with a satisfaction threshold of 0.4.
''')

if not algoADD.dd_holds():
        print(f'''This {COLOR_CODES["red"]}{'ADD'} doesn`t hold{COLOR_CODES["nocolor"]}, error threshold is {COLOR_CODES["red"]}{algoADD.get_error()}{COLOR_CODES["nocolor"]}.\n''')

print('''In this case, there are no violations; rather, the fraction of pairs 
satisfying the RHS distance constraint among those that satisfy the 
LHS constraint does not exceed the satisfaction_threshold.
''')

print('-' * 49)

lhs2 = [desbordante.dd_verification.DF("education_level", 0, 0)]
rhs2 = [desbordante.dd_verification.DF("salary", 0, 1990)]
dd2 = desbordante.dd.DD(lhs2, rhs2)


print(f'''Example #3

Also, consider an ADD with a satisfaction threshold of zero. In this
case, any ADD will hold because the requirement is for at least a 
zero fraction of pairs with minimal distance to satisfy the RHS.

Consider the following dependency:

{COLOR_CODES["yellow"]}{dd2.__str__()}{COLOR_CODES["nocolor"]}
''')
algoDD.execute(dd=dd2)
algoADD.execute(dd=dd2, satisfaction_threshold=0)

print_holds(algoDD, TABLE0, False)
print('''Also, consider an ADD with a satisfaction threshold of 0.
''')

if algoADD.dd_holds():
        print(f'''This {COLOR_CODES["green"]}{'ADD'} holds{COLOR_CODES["nocolor"]}, error threshold is {COLOR_CODES["green"]}{algoADD.get_error()}{COLOR_CODES["nocolor"]}.\n''')

print('''In such a scenario, the ADD has no practical meaning, because 
even a DD with an error threshold equal to 1 would be satisfied.''')

print('-' * 49)

print(f'''Example #4
Consider the following ADD with satisfaction threshold of 0.4:

{COLOR_CODES["yellow"]}{dd.__str__()}{COLOR_CODES["nocolor"]}
''')

algoADD.execute(dd=dd, satisfaction_threshold=0.4)

print_holds(algoADD, TABLE0)

print('''In practice, it would be logical to expect that for the same 
education level, there would be a sufficiently large proportion
of individuals with a minimal difference in salary level. 

In our case, we assume that at least a 0.4 fraction of 
individuals have a minimal salary difference. Since validation 
results proved this assumption incorrect, so we will extend our
table with data from additional individuals.''')

TABLE1 = 'examples/datasets/add_example1.csv'
df1 = pd.read_csv(TABLE1)

print('\nBelow is the extended table:\n')

print(df1, end='\n\n')

algoADD1 = desbordante.add_verification.algorithms.Default()
algoADD1.load_data(table=(TABLE1, ',', True))
algoADD1.execute(dd=dd, satisfaction_threshold=0.4)

if algoADD1.dd_holds():
        print(f'''Now this {COLOR_CODES["green"]}{'ADD'} holds{COLOR_CODES["nocolor"]}, error threshold is {COLOR_CODES["green"]}{algoADD1.get_error()}{COLOR_CODES["nocolor"]}.\n''')
