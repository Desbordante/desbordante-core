import operator

import desbordante
import pandas
from tabulate import tabulate


COLOR_CODES = {
    'bold_green': '\033[1;32m',
    'bold_yellow': '\033[1;33m',
    'bold_blue': '\033[1;34m',
    'green': '\033[32m',
    'yellow': '\033[33m',
    'default': '\033[0m',
    'green_bg': '\033[1;42m',
    'red_bg': '\033[1;41m',
    'blue_bg': '\033[1;46m',
    'default_bg': '\033[1;49m'
}

TABLE = 'examples/datasets/cargo_march.csv'
SIMPLE_TABLE = 'examples/datasets/player_stats.csv'
HEADER = 0
SEPARATOR = ','
P_FUZZ = 0.85
FUZZINESS = 0.2
BUMPS_LIMIT = 0
WEIGHT = 0.1
REDUCED_WEIGHT =  0.05
BIN_OPERATION = '-'
AC_SEED = 11
ITERATIONS_LIMIT = 4
OPERATIONS = {
    '+': (operator.add, 'Sum'),
    '-': (operator.sub, 'Difference'),
    '*': (operator.mul, 'Product'),
    '/': (operator.truediv, 'Ratio'),
}


def print_table(table_, headers_ = None, title_ = None):
    if title_ is not None:
        print(title_)

    print(tabulate(table_, headers=headers_, tablefmt='pipe', showindex=False, stralign='center'))
    print()


def run_ac_mining(table_, columns_, p_fuzz_=P_FUZZ, fuzziness_=FUZZINESS,
         bumps_limit_=BUMPS_LIMIT, weight_=WEIGHT, bin_operation_=BIN_OPERATION,
         ac_seed_=AC_SEED, iterations_limit_=ITERATIONS_LIMIT):
    operation, operation_name = OPERATIONS[bin_operation_]

    algo = desbordante.ac.algorithms.Default()

    table_scoped = table_[columns_]

    algo.load_data(table=table_scoped)

    algo.execute(p_fuzz=p_fuzz_, fuzziness=fuzziness_, bumps_limit=bumps_limit_, weight=weight_,
            bin_operation=bin_operation_, ac_seed=ac_seed_, iterations_limit=iterations_limit_)
    print()
    ac_ranges = algo.get_ac_ranges()
    for ac_range in ac_ranges:
        l_col = table_scoped.columns[ac_range.column_indices[0]]
        r_col = table_scoped.columns[ac_range.column_indices[1]]
        print(f'{COLOR_CODES["green_bg"]}Discovered ranges{COLOR_CODES["default"]} ' +
              f'for ({l_col} {bin_operation_} {r_col}) are:')
        print(ac_range.ranges)

    ac_exceptions = algo.get_ac_exceptions()
    print()
    print(f'Rows in which the result of the chosen operation ({bin_operation_}) is ' +
          f'{COLOR_CODES["red_bg"]}outside{COLOR_CODES["default_bg"]} of discovered ranges:')
    for ac_exception in ac_exceptions:
        id_range, column_0, column_1 = table_.iloc[ac_exception.row_index]
        print(f'{COLOR_CODES["blue_bg"]}id: {id_range}{COLOR_CODES["default_bg"]}')
        print(f'{columns_[1]}: {column_1}')
        print(f'{columns_[0]}: {column_0}')
        print(f'{operation_name}: {operation(column_0, column_1)}')
        print()
    if len(ac_exceptions) == 0:
        print(f'{COLOR_CODES["blue_bg"]}None{COLOR_CODES["default_bg"]}\n')


print(
'''This example is dedicated to Fuzzy Algebraic Constraints (AC). The definition and algorithm 
are based on article "B-HUNT: Automatic Discovery of Fuzzy Algebraic Constraints in Relational 
Data" by Paul G. Brown & Peter J. Haas presented at VLDB in 2003.
'''
)

print(
'''First of all, let's figure out what AC is. However, to avoid going too deep, we will give you 
a simple definition without formalization. AC represents the results of applying binary 
operations between two table columns, with values grouped into meaningful intervals. 
Let's illustrate this with an example.
'''
)

print('We have a table examples/datasets/player_stats.csv with the following data:')

table_for_example = pandas.read_csv(SIMPLE_TABLE, sep=SEPARATOR, header=HEADER)

print_table(table_for_example, headers_=['id', 'Strength', 'Agility'])
print('Let\'s apply binary operation "+" to the Strength and Agility columns and observe the results.')

run_ac_mining(table_for_example, ['Strength', 'Agility'], bin_operation_='+')

print(
'''As shown, the sum of Strength and Agility falls within either the (4, 5) or (22, 24) ranges. 
This pattern may emerge because player characters with similar combined attribute values 
likely belong to the same tier.

'''
)

print('To run the algorithm, you must configure the parameters below:')

print(
f'''For {COLOR_CODES["bold_blue"]}binary arithmetic operations{COLOR_CODES["default"]}, you can use four options:{COLOR_CODES["default_bg"]}
"+"
"-"
"*"
"/"
{COLOR_CODES["default"]}'''
)

print(
f'''Furthermore, AC mining algorithm provides five {COLOR_CODES["bold_blue"]}parameters for setting up execution{COLOR_CODES["default"]}:
{COLOR_CODES["default_bg"]}weight
fuzziness
p_fuzz
bumps_limit
iterations_limit
ac_seed
{COLOR_CODES["default"]}'''
)

print(
f'''{COLOR_CODES["green_bg"]}Weight{COLOR_CODES["default"]} accepts values in the range (0, 1]. 
Values closer to 1 force the algorithm to produce fewer larger intervals (up to a single 
interval covering all values).
Values closer to 0 force the algorithm to produce smaller intervals.
'''
)

print(
f'''{COLOR_CODES["green_bg"]}Fuzziness{COLOR_CODES["default"]} belongs to (0,1) range while {COLOR_CODES["green_bg"]}p_fuzz{COLOR_CODES["default"]} belongs to [0,1] range. These parameters 
control precision and the number of considered rows.
Fuzziness values closer to 0 and p_fuzz values closer to 1 force the algorithm to include 
more rows (higher accuracy).
Fuzziness values closer to 1 and p_fuzz values closer to 0 force the algorithm to include 
fewer rows (higher chance of skipping rows, lower precision, but faster execution).
'''
)

print(
f'''{COLOR_CODES["green_bg"]}Bumps_limit{COLOR_CODES["default"]} accepts only natural numbers from the range [1, inf) and limits the number of 
intervals for all column pairs. To set bumps_limit to inf you should use the 0 value.
'''
)

print(
f'''{COLOR_CODES["green_bg"]}Iterations_limit{COLOR_CODES["default"]} accepts only natural numbers. 
Lower values (close to 1) reduce accuracy due to algorithm performing fewer iterations.
'''
)

print(
f'''{COLOR_CODES["green_bg"]}AC_seed{COLOR_CODES["default"]} accepts only natural numbers. 
B-HUNT is a randomized algorithm that accepts the seed parameter (AC_seed). Fixing this 
parameter ensures reproducible results, which are necessary for verifying results during 
testing of the algorithm. Furthermore, we need to fix it in this example for demonstration 
purposes; otherwise, we may obtain a different number of intervals with different boundaries 
that will not correspond to the text we wrote for our output.
'''
)

print(
f'''Let\'s proceed to a visual example. We will use dataset from this path: 
{TABLE}.
For default parameters we will use those values:
binary operation is "{BIN_OPERATION}", weight - {WEIGHT}, fuzziness - {FUZZINESS}, p_fuzz - {P_FUZZ}, bumps_limit - {BUMPS_LIMIT}, 
iterations_limit - {ITERATIONS_LIMIT}, AC_seed - {AC_SEED}.
'''
)

print('Let\'s see the result of the algorithm with these parameters.')

table_for_example = pandas.read_csv(TABLE, sep=SEPARATOR, header=HEADER)
run_ac_mining(table_for_example, ['Delivery date', 'Dispatch date'])

print(
f'''You can see that the algorithm creates two intervals for the binary operation "-": (2-7) and 
(15-22). This means that the difference between the dispatch date and delivery date always 
falls within these intervals, except for three rows where the difference lies outside the 
discovered ranges. From this, we can infer that:
{COLOR_CODES["bold_yellow"]}Packages for some addresses are typically delivered within 7 days.{COLOR_CODES["default"]}
{COLOR_CODES["bold_yellow"]}Packages for some addresses take up to 22 days.{COLOR_CODES["default"]}
'''
)

print(
'''Why these two intervals? To answer this question, more context is needed; that is, we should 
look into the underlying data. We can imagine several reasons for this result, such as: 1) 
nearby addresses versus far addresses; 2) air shipping versus regular shipping.
'''
)

print(
'''There are three parcels that fall outside of these delivery intervals. Why? This is a point 
for further investigation, which requires additional context. There are many possible reasons 
for this: 1) on these dates there was a workers' strike in some regions; or 2) an incorrect 
address was specified, which increased the delivery time; or 3) it is just a typo in the table.
'''
)

print('Now we reduce the value of the parameter weight to 0.05.')

table_for_example = pandas.read_csv(TABLE, sep=SEPARATOR, header=HEADER)
run_ac_mining(table_for_example, ['Delivery date', 'Dispatch date'], weight_=REDUCED_WEIGHT)

print(
'''You can see that the number of intervals increases, and there is no longer any data outside of 
the discovered ranges.
However, with this number of intervals, it is difficult to draw immediate conclusions about the 
delivery date. In this case, a detailed analysis of other attributes might enable more meaningful 
predictions for delivery times. For example, it may be a good idea to partition data by the region 
attribute (or by month/quarter) and consider each partition individually. 
'''
)

print(
'''Another option is to try to find a parameter combination that will result in a smaller number of 
intervals. Next, remember that the algorithm is randomized (unless you run it with the exact 
settings) — it can skip some rows, so you can also try to alter the seed.
'''
)

print(
'''Finally, cleaning up the data by removing duplicate and incomplete rows might also help.
Thus, the quantity and quality of the intervals are the user's responsibility. It may take several 
attempts to achieve something interesting. Experiment!'''
)
