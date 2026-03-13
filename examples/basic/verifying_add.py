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
This relaxation allows focusing on meaningful patterns while filtering out 
outliers and data errors.
      

Example #1 

First, let's understand how satisfaction threshold affects ADD validation.
We'll use an employee salary dataset to demonstrate.
''')

TABLE0 = 'examples/datasets/add_example.csv'
df0 = pd.read_csv(TABLE0)

lhs = [desbordante.add_verification.DF("education_level", 0, 0)]
rhs = [desbordante.add_verification.DF("salary", 0, 15000)]
dd = desbordante.dd.DD(lhs, rhs)

print("-" * 42)
print(df0)
print("-" * 42)

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
threshold eps = 1.0:
''')

algoADD = desbordante.add_verification.algorithms.Default()
algoADD.load_data(table=(TABLE0, ',', True))
algoADD.execute(dd=dd, satisfaction_threshold=1.0)
print_holds(algoADD, TABLE0)

print("Both tests produce exactly the same outcome.")

lhs = [desbordante.add_verification.DF("education_level", 0, 0)]
rhs = [desbordante.add_verification.DF("salary", 0, 200000)]
dd = desbordante.dd.DD(lhs, rhs)

print(f'''
Now let's see what happens when we relax the constraint by expanding 
the salary interval from 15000 to 200000. Consider the DD: 
      
{COLOR_CODES["yellow"]}{dd.__str__()}{COLOR_CODES["nocolor"]}

Let's check whether this DD holds: 
''')

algoDD = desbordante.dd_verification.algorithms.Default()
algoDD.load_data(table=(TABLE0, ',', True))
algoDD.execute(dd=dd)
print_holds(algoDD, TABLE0, is_ADD=False)

print('''Now let's test the same dependency as an ADD with satisfaction 
threshold eps = 1.0:
''')

algoADD = desbordante.add_verification.algorithms.Default()
algoADD.load_data(table=(TABLE0, ',', True))
algoADD.execute(dd=dd, satisfaction_threshold=1.0)
print_holds(algoADD, TABLE0)

print(f'''As we can see, both tests now succeed. This confirms that DD and 
ADD with eps = 1.0 behave identically.

      
Example #2
      
Now let's examine a climate dataset containing US cities with their
average temperatures and precipitation levels.
''')  

TABLE1 = 'examples/datasets/temperature1_add.csv'
TABLE2 = 'examples/datasets/temperature2_add.csv'
df1 = pd.read_csv(TABLE1)
df2 = pd.read_csv(TABLE2)

lhs = [desbordante.add_verification.DF("state", 0, 0)]
rhs = [desbordante.add_verification.DF("avg_temperature_F", 0, 10)]
dd = desbordante.dd.DD(lhs, rhs)

print("-" * 72)
print(df1)
print("-" * 72)

print(f'''
Consider the following ADD with satisfaction threshold = 1.0:

{COLOR_CODES["yellow"]}{dd.__str__()}{COLOR_CODES["nocolor"]}

This dependency means that for any two cities within the same state,
the difference in their average temperatures should not exceed 10 degrees.

Let's check whether this ADD holds:  
''')
algo1 = desbordante.add_verification.algorithms.Default()
algo1.load_data(table=(TABLE1, ',', True))
algo1.execute(dd=dd, satisfaction_threshold=1.0)
print_holds(algo1, TABLE1)

print(f'''The detected violations correspond to real geographic effects rather than
data quality issues. Although the dependency is violated, such violations
are better interpreted as exceptions and should not have a strong influence
on the overall pattern.
      
Let's check the same ADD with satisfaction threshold = 0.85:   
''')
algo1.execute(dd=dd, satisfaction_threshold=0.85)
print_holds(algo1, TABLE1)

print("Now let's add more California cities and see how the error changes:")

print("-" * 72)
print(df2)
print("-" * 72)

algo2 = desbordante.add_verification.algorithms.Default()
algo2.load_data(table=(TABLE2, ',', True))
algo2.execute(dd=dd)
print_holds(algo2, TABLE2)

print('''As we can see, adding more cities increased the error rate.
This happened because each new city creates additional violating pairs
with the existing outlier, even though the new cities themselves
follow the temperature pattern of the state.
      
      
For a detailed example of exact differential dependency validation, 
see: examples/basic/verifying_dd.py. That file contains demonstrations
of error correction using both DDs and ADDs.''')
