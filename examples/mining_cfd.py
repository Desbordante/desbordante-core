"""CFD mining example with visualization using Desbordante algorithms."""

from collections import defaultdict

import desbordante
import numpy as np
import pandas

# CFD Discovery parameters:
TABLE_PATH = 'examples/datasets/play_tennis.csv'
MINIMUM_SUPPORT = 8
MINIMUM_CONFIDENCE = 0.7
MAXIMUM_LHS_COUNT = 3
GREEN_BG_CODE = '\033[1;42m'
GREEN_FG_CODE = '\033[1;32m'
RED_BG_CODE = '\033[1;41m'
DEFAULT_BG_CODE = '\033[1;49m'
DEFAULT_FG_CODE = '\033[1;37m/'


def items_to_indices(items):
    return [item.attribute for item in items]


def get_supported_mask(pattern, table):
    table_height = table.shape[0]
    true_array = np.full(shape=table_height, fill_value=True, dtype=np.bool_)
    mask = pandas.Series(true_array)
    for cfd_item in pattern:
        item_value = cfd_item.value
        if item_value is None:
            continue
        column_name = table.columns[cfd_item.attribute]
        mask &= table[column_name].astype(str).str.lower() == item_value.lower()
    return mask


# returns a dictionary
#   key: a tuple of attribute values (all attributes from lhs)
#   value: indices of all rows with those attribute values from supported_df
def make_lhs_to_row_nums(lhs, supported_df):
    lhs_to_row_nums = defaultdict(list)
    lhs_column_indices = items_to_indices(lhs)
    for row_num, row_series in supported_df.iterrows():
        key = tuple(row_series.iloc[lhs_column_indices])
        lhs_to_row_nums[key].append(row_num)
    return lhs_to_row_nums


def validate_cfd(lhs, rhs, table):
    supported_mask = get_supported_mask(lhs, table)
    supported_df = table[supported_mask]
    lhs_to_row_nums = make_lhs_to_row_nums(lhs, supported_df)
    rows_satisfying_cfd = []

    # add rows with identical lhs to rows_satisfying_cfd if they have the most frequent rhs
    for lhs_tuple, row_nums_with_lhs in lhs_to_row_nums.items():
        # fill rhs_to_row_nums
        rhs_to_row_nums = defaultdict(list)
        for row_num in row_nums_with_lhs:
            key = table.iloc[row_num, rhs.attribute]
            rhs_to_row_nums[key].append(row_num)
        row_nums_with_most_frequent_rhs = max(rhs_to_row_nums.values(), key=len)
        rows_satisfying_cfd.extend(row_nums_with_most_frequent_rhs)

    return rows_satisfying_cfd, supported_df.index


def visualize_cfd(cfd, table):
    print('CFD:')
    print(cfd, ":\n")

    rows_satisfying_cfd, supported_rows = validate_cfd(cfd.lhs_items, cfd.rhs_item, table)

    header, *rows = table.to_string().splitlines()
    print(DEFAULT_BG_CODE, header)
    for index, row in enumerate(rows):
        if index not in supported_rows:
            color_code = DEFAULT_BG_CODE
        elif index in rows_satisfying_cfd:
            color_code = GREEN_BG_CODE
        else:
            color_code = RED_BG_CODE
        print(color_code, row, DEFAULT_BG_CODE)

    support = len(supported_rows)
    num_rows_satisfy_cfd = len(rows_satisfying_cfd)
    print("lhs count: ", len(cfd.lhs_items))
    print("support: ", support, GREEN_BG_CODE, " ", RED_BG_CODE, " ", DEFAULT_BG_CODE)
    print("confidence: ", GREEN_FG_CODE, num_rows_satisfy_cfd, DEFAULT_FG_CODE, support,
          " = ", format(num_rows_satisfy_cfd / support, '.4f'))


if __name__ == '__main__':
    tableDF = pandas.read_csv(TABLE_PATH)
    algo = desbordante.cfd.algorithms.Default()
    algo.load_data(table=tableDF)
    algo.execute(cfd_minconf=MINIMUM_CONFIDENCE,
                 cfd_minsup=MINIMUM_SUPPORT,
                 cfd_max_lhs=MAXIMUM_LHS_COUNT)
    result = algo.get_cfds()
    print("options: \nMINIMUM SUPPORT =", MINIMUM_SUPPORT,
          ", MINIMUM CONFIDENCE =", MINIMUM_CONFIDENCE,
          ", MAXIMUM LHS COUNT =", MAXIMUM_LHS_COUNT)
    print("displaying the first five (or fewer) discovered CFDs:\n")
    for cfd in result[:5]:
        visualize_cfd(cfd, tableDF)
        print("\n\n")
