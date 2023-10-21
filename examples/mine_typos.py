"""Typo mining example using Desbordante algorithms."""

from itertools import groupby, islice

from colorama import Style, Fore
from jellyfish import levenshtein_distance
import desbordante
import pandas

# Value cluster filtering parameters.
RADIUS = 3
RATIO = 0.1

# Algorithm that finds exact FDs and its config.
EXACT_ALGORITHM_TYPE = desbordante.HyFD
EXACT_ALGO_CONFIG = {}

# Algorithm that finds approximate FDs and its config.
APPROXIMATE_ALGORITHM_TYPE = desbordante.Pyro
ERROR = 0.005  # Highest error for almost holding FDs.
APPROXIMATE_ALGO_CONFIG = {'error': ERROR}

# Parameters for pandas.read_csv(...).
DATASET_PATH = 'datasets/Workshop.csv'
HEADER = 0
SEPARATOR = ','

# Index of the almost holding FD. Chosen in advance purely for
# demonstration purposes. In a real usage scenario this should be a
# choice for the user.
FD_INDEX = 2

assert APPROXIMATE_ALGO_CONFIG['error'] > 0.0, 'Typo mining relies on non-zero error'
assert EXACT_ALGO_CONFIG.get('error', 0.0) == 0.0, 'Error must be 0 for precise algorithm'

# Variables to simplify the configuration string construction below.
EXACT_ALGORITHM = EXACT_ALGORITHM_TYPE.__name__
APPROXIMATE_ALGORITHM = APPROXIMATE_ALGORITHM_TYPE.__name__

# A message containing all variables used by this usage scenario, to be
# displayed to the user.
CONFIG_STRING = f"""Starting typo discovery scenario with parameters:
{RADIUS=}
{RATIO=}
{ERROR=}
{DATASET_PATH=}
{EXACT_ALGORITHM=}
{APPROXIMATE_ALGORITHM=}
{HEADER=}
{SEPARATOR=}"""


def configure_dataframe_print():
    pandas.set_option('display.max_columns', None)
    pandas.set_option('display.width', None)
    pandas.set_option('display.max_colwidth', None)


def get_squashed_sorted_clusters(dataset: pandas.DataFrame, lhs_indices, rhs_index):
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


def fd_to_string(dataset: pandas.DataFrame, fd: tuple[tuple[int], int]):
    def get_col_name(col_index):
        return str(dataset.columns[col_index])

    lhs_indices, rhs_index = fd
    return f'( {" ".join(map(get_col_name, lhs_indices))} ) -> {get_col_name(rhs_index)}'


def get_result_set(df, algo_type, algo_config):
    algo = algo_type()
    algo.load_data(df, **algo_config)
    algo.execute(**algo_config)
    return {(tuple(fd.lhs_indices), fd.rhs_index) for fd in algo.get_fds()}


def make_display_df(squashed_sorted_clusters, original_df, lhs_indices, rhs_index):
    display_rows = []
    for lhs, value_info in squashed_sorted_clusters:
        for value, count in value_info:
            display_rows.append((count, *lhs, value))
    return pandas.DataFrame(display_rows, columns=['rows count']
                            + [original_df.columns[col] for col in lhs_indices]
                            + [original_df.columns[rhs_index]])


def print_display_df(display_df):
    df_lines = display_df.to_string(index=False).splitlines()
    print(df_lines[0])
    print(Fore.GREEN + df_lines[1] + Style.RESET_ALL)
    print(Fore.RED + '\n'.join(islice(df_lines, 2)) + Style.RESET_ALL)
    print()


def get_typo_candidates_df(df, display_df):
    typo_candidate_rows = []
    typo_candidate_row_indices = []

    assert all(col_name.replace('_', '').isalnum() for col_name in df.columns)
    for index, row in display_df.iterrows():
        # Brittle, but good enough for an example. Breaks when there are
        # unusual characters like a backslash in the column name.
        query_data = df.query('and'.join(
            f'`{name}` == {value!r}' for name, value in islice(row.items(), 1, None))).head(1)
        typo_candidate_rows.append(query_data.values[0])
        typo_candidate_row_indices.append(query_data.index.values[0])
    return pandas.DataFrame(typo_candidate_rows, columns=df.columns, index=typo_candidate_row_indices)


def main():
    configure_dataframe_print()

    print(CONFIG_STRING)
    print()

    df = pandas.read_csv(DATASET_PATH, sep=SEPARATOR, header=HEADER, na_filter=False, dtype='string')
    print('Dataset sample:')
    print(df)
    print()

    print('Searching for almost holding FDs...')
    print()
    holding_fds = get_result_set(df, EXACT_ALGORITHM_TYPE, EXACT_ALGO_CONFIG)
    close_fds = get_result_set(df, APPROXIMATE_ALGORITHM_TYPE, APPROXIMATE_ALGO_CONFIG)
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
