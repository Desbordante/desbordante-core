"""Typo mining example using Desbordante algorithms."""

from itertools import groupby, islice

from colorama import Style, Fore
from jellyfish import levenshtein_distance
import desbordante as desb
import pandas as pd

RADIUS = 3
RATIO = 0.1
ERROR = 0.005
EXACT_ALGORITHM = 'FastFDs'
EXACT_CONFIG = {}
APPROXIMATE_ALGORITHM = 'Pyro'
APPROXIMATE_CONFIG = {'error': ERROR}
DATASET_PATH = 'datasets/Workshop.csv'
HEADER = 0
SEPARATOR = ','
FD_INDEX = 2
assert 'error' not in EXACT_CONFIG or EXACT_CONFIG[
    'error'] == 0.0, 'Error must be 0 for precise algorithm'

CONFIG_STRING = f"""Starting typo discovery scenario with parameters:
{RADIUS=}
{RATIO=}
{ERROR=}
{DATASET_PATH=}
{EXACT_ALGORITHM=}
{APPROXIMATE_ALGORITHM=}
{HEADER=}
{SEPARATOR=}"""


def setup_pandas_print():
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    pd.set_option('display.max_colwidth', None)


def get_squashed_sorted_clusters(dataset: pd.DataFrame, lhs_indices, rhs_index):
    def get_lhs(row_count_pair):
        row, _ = row_count_pair
        return row[:-1]

    def get_rhs(row):
        return row[-1]

    def count_key(rhs_count_pair):
        rhs, count = rhs_count_pair
        # Sort descending on count, ascending on rhs value
        return -count, rhs

    fd_columns = [dataset.columns[col_num] for col_num in lhs_indices]
    fd_columns.append(dataset.columns[rhs_index])
    value_counts = dataset.value_counts(fd_columns, dropna=False)
    # Rows with the same LHS now end up next to each other and can be
    # grouped together with groupby. But inside each group rows may not
    # be sorted by the number of their occurrences.
    value_counts.sort_index(inplace=True)
    lhs_groups = ((lhs, row_count_pairs) for lhs, row_count_iter in
                  groupby(value_counts.items(), key=get_lhs) if
                  # Exclude instances where FD is not violated.
                  len(row_count_pairs := tuple(row_count_iter)) > 1)
    # The final step is transforming lhs groups in the form of
    # (lhs, (((*lhs, rhs_value), count), ...)) to the form
    # (lhs, ((rhs_value, count), ...)) and sorting them by the number
    # of occurrences in the table.
    return [(lhs, sorted(((get_rhs(row), count) for row, count in row_count_pairs), key=count_key))
            for lhs, row_count_pairs in lhs_groups]


def number_metric(a, b):
    return abs(a - b)


def string_metric(a, b):
    return levenshtein_distance(str(a), str(b))


def filter_radius(squashed_sorted_clusters, metric) -> list:
    def is_value_close(value_count_pair):
        value, _ = value_count_pair
        return metric(most_common_value, value) < RADIUS

    filtered = []
    for lhs, value_data in squashed_sorted_clusters:
        most_common_value, _ = value_data[0]
        close_value_pairs = list(filter(is_value_close, islice(value_data, 1, None)))
        if close_value_pairs:
            filtered.append((lhs, [value_data[0]] + close_value_pairs))
    return filtered


def filter_ratio(squashed_sorted_clusters):
    def few_deviations(squashed_sorted_cluster):
        _, value_info = squashed_sorted_cluster
        _, most_common_count = value_info[0]
        total_values = sum(number for _, number in value_info)
        deviating_values = total_values - most_common_count
        return deviating_values / total_values < RATIO

    return list(filter(few_deviations, squashed_sorted_clusters))


def filter_squashed_sorted_clusters(squashed_sorted_clusters):
    try:
        squashed_sorted_clusters = filter_radius(squashed_sorted_clusters, number_metric)
    except TypeError:
        squashed_sorted_clusters = filter_radius(squashed_sorted_clusters, string_metric)
    return filter_ratio(squashed_sorted_clusters)


def fd_to_string(dataset: pd.DataFrame, fd: tuple[tuple[int], int]):
    def get_col_name(col_index):
        return str(dataset.columns[col_index])

    lhs_indices, rhs_index = fd
    return f'( {" ".join(map(get_col_name, lhs_indices))} ) -> {get_col_name(rhs_index)}'


def get_result_set(df, algo_name, algo_config):
    algo = getattr(desb, algo_name)()
    algo.load_data(df, **algo_config)
    algo.execute(**algo_config)
    return {(tuple(fd.lhs_indices), fd.rhs_index) for fd in algo.get_fds()}


def make_display_df(squashed_sorted_clusters, original_df, lhs_indices, rhs_index):
    display_rows = []
    for lhs, value_info in squashed_sorted_clusters:
        for value, count in value_info:
            display_rows.append((count, *lhs, value))
    return pd.DataFrame(display_rows, columns=['rows count']
                        + [original_df.columns[col] for col in lhs_indices]
                        + [original_df.columns[rhs_index]])


def print_display_df(display_df):
    df_lines = display_df.to_string(index=False).splitlines()
    print(df_lines[0])
    print(Fore.GREEN + df_lines[1] + Style.RESET_ALL)
    print(Fore.RED + '\n'.join(df_lines[2:]) + Style.RESET_ALL)
    print()


def get_typo_candidates_df(df, display_df):
    typo_candidate_rows = []
    typo_candidate_row_indices = []
    for index, row in display_df.iterrows():
        # Brittle, but good enough for an example. Breaks when there are
        # unusual characters like a backslash in the column name.
        query_data = df.query('and'.join(
            f'`{name}` == {value!r}' for name, value in islice(row.items(), 1, None))).head(1)
        typo_candidate_rows.append(query_data.values[0])
        typo_candidate_row_indices.append(query_data.index.values[0])
    return pd.DataFrame(typo_candidate_rows, columns=df.columns, index=typo_candidate_row_indices)


def main():
    setup_pandas_print()

    print(CONFIG_STRING)
    print()

    df = pd.read_csv(DATASET_PATH, sep=SEPARATOR, header=HEADER, na_filter=False, dtype='string')
    print('Dataset sample:')
    print(df)
    print()

    print('Searching for almost holding FDs...')
    print()
    holding_fds = get_result_set(df, EXACT_ALGORITHM, EXACT_CONFIG)
    close_fds = get_result_set(df, APPROXIMATE_ALGORITHM, APPROXIMATE_CONFIG)
    almost_holding_fds = sorted(close_fds - holding_fds)
    print('Found! Almost holding FDs:')
    print('\n'.join(map(lambda fd: fd_to_string(df, fd), almost_holding_fds)))
    print()

    print(f'Selecting FD with index {FD_INDEX}:')
    lhs_indices, rhs_index = almost_holding_fds[FD_INDEX]
    squashed_sorted_clusters = filter_squashed_sorted_clusters(
        get_squashed_sorted_clusters(df, lhs_indices, rhs_index))

    if not squashed_sorted_clusters:
        print('Nothing found. Try another FD or relax restrictions (radius, ratio, error).')
        return

    display_df = make_display_df(squashed_sorted_clusters, df, lhs_indices, rhs_index)
    print_display_df(display_df)
    print('Typo candidates and context:')
    print(get_typo_candidates_df(df, display_df).to_string())


if __name__ == '__main__':
    main()
