import desbordante
import pandas
import operator
from datetime import datetime

# Note that dates in the given dataset must be in the format Y-M-D
TABLE = 'examples/datasets/ACShippingDates.csv'
HEADER = 0
SEPARATOR = ','
P_FUZZ = 0.7
FUZZINESS = 0.3
BUMPS_LIMIT = 0
WEIGHT = 0.1
BIN_OPERATION = '-'
AC_SEED = 11
ITERATIONS_LIMIT = 10
OPERATIONS = {
    '+': (operator.add, 'Sum'),
    '-': (operator.sub, 'Difference'),
    '*': (operator.mul, 'Product'),
    '/': (operator.truediv, 'Ratio'),
}
operation, operation_name = OPERATIONS[BIN_OPERATION]

algo = desbordante.ACAlgorithm()

df = pandas.read_csv(TABLE, sep=SEPARATOR, header=HEADER)
df_without_id = df[['deliveryDate', 'shipDate']]

algo.load_data(df=df_without_id)

algo.execute(p_fuzz=P_FUZZ, fuzziness=FUZZINESS, bumps_limit=BUMPS_LIMIT, weight=WEIGHT,
             bin_operation=BIN_OPERATION, ac_seed=AC_SEED, iterations_limit=ITERATIONS_LIMIT)

ac_ranges = algo.get_ac_ranges()
for ac_range in ac_ranges:
    l_col = df_without_id.columns[ac_range.column_indices[0]]
    r_col = df_without_id.columns[ac_range.column_indices[1]]
    print(f'Discovered ranges for ({l_col} {BIN_OPERATION} {r_col}) are:')
    print(ac_range.ranges)

ac_exceptions = algo.get_ac_exceptions()
print()
print(f'Rows in which the result of the chosen operation ({BIN_OPERATION}) is outside of discovered ranges:')
for ac_exception in ac_exceptions:
    id, delievery_date, ship_date = df.iloc[ac_exception.row_index]
    print(f'id: {id}')
    print(f'Shipping date : {ship_date}')
    print(f'Delivery date: {delievery_date}')
    date1 = datetime.strptime(ship_date, '%Y-%m-%d').date()
    date2 = datetime.strptime(delievery_date, '%Y-%m-%d').date()
    print(f'{operation_name}: {operation(date2, date1).days}')
    print()
