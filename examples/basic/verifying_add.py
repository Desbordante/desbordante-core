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

def print_holds(result: bool) -> None:
    if result:
        print(f'This {COLOR_CODES["green"]}DD holds.{COLOR_CODES["nocolor"]}\n')
    else:
        print(f'This {COLOR_CODES["red"]}DD doesn`t hold.{COLOR_CODES["nocolor"]}\n')

        print(f"{COLOR_CODES['cyan']}Violations found:{COLOR_CODES['nocolor']}")
        highlights = algo.get_highlights()
        with open(TABLE, newline='') as f:
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



TABLE = 'examples/datasets/temperature_add.csv'
df = pd.read_csv(TABLE)

lhs = [desbordante.add_verification.DF("state", 0, 0)]
rhs = [desbordante.add_verification.DF("avg_temperature_F", 0, 10)]
dd = desbordante.dd.DD(lhs, rhs)

algo = desbordante.add_verification.algorithms.DDVerifier()
algo.load_data(table=(TABLE, ',', True))

print('''
This example demonstrates validation of approximate differential dependencies.
For a detailed example of standard differential dependency validation, 
see: examples/basic/verifying_dd.py
      
Differential dependencies (DDs) define constraints on differences between
attribute values in a table. Approximate DDs relax these constraints by allowing
a limited number of violations. This makes it possible to focus on meaningful
patterns while filtering out outliers and data errors.
      
In this example, we examine a dataset with average temperature and precipitation
data for U.S. cities.
''')  

print("-" * 72)
print(df)
print("\n" + "-" * 72)

print(f'''
Consider the following DD:

{COLOR_CODES["yellow"]}{dd.__str__()}{COLOR_CODES["nocolor"]}

This dependency means that for any two cities within the same state,
the difference in their average temperatures should not exceed 10 degrees.

Let's check whether this DD holds:  
''')
algo.execute(dd=dd)
print_holds(algo.dd_holds())

print(f'''
The detected violations correspond to real geographic effects rather than
data quality issues. Although the dependency is violated, such violations are better
interpreted as exceptions and should not have a strong influence on the overall pattern.
      
Let's check the same DD with an 85% satisfaction threshold:   
''')
algo.execute(dd=dd, satisfaction_threshold=0.85)
print_holds(algo.dd_holds())