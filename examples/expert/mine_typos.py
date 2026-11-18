"""Typo mining example using Desbordante algorithms."""

from functools import reduce
from itertools import islice
import operator

from colorama import Style, Fore
from jellyfish import levenshtein_distance
from collections import Counter
import desbordante
import pandas

# Value cluster filtering parameters.
RADIUS = 3
RATIO = 0.1

# Algorithm that finds exact FDs and its config.
EXACT_ALGORITHM_TYPE = desbordante.fd.algorithms.Default
EXACT_ALGO_CONFIG = {}

# Algorithm that finds approximate FDs and its config.
APPROXIMATE_ALGORITHM_TYPE = desbordante.afd.algorithms.Pyro
ERROR = 0.005  # Highest error for almost holding FDs.
APPROXIMATE_ALGO_CONFIG = {'error': ERROR}

# Parameters for pandas.read_csv(...).
DATASET_PATH = 'examples/datasets/Workshop.csv'
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


def get_squashed_sorted_filtered_clusters(dataset: pandas.DataFrame, lhs_indices, rhs_index):
    def count_key(rhs_count_pair):
        rhs, count = rhs_count_pair
        # Sort descending on count, ascending on rhs value
        return -count, rhs

    fd_verifier = desbordante.fd_verification.algorithms.FDVerifier()
    fd_verifier.load_data(table=dataset)
    fd_verifier.execute(lhs_indices=lhs_indices, rhs_indices=[rhs_index])

    if fd_verifier.fd_holds():
        return []

    highlights = fd_verifier.get_highlights()

    squashed_clusters = []
    for current_highlight in highlights:
        if current_highlight.most_frequent_rhs_value_proportion <= 1.0 - RATIO:
            continue

        cluster_row_indices = current_highlight.cluster

        rhs_values = [str(dataset.iat[row_index, rhs_index]) for row_index in cluster_row_indices]
        rhs_counts = Counter(rhs_values)

        sorted_rhs_counts = sorted(rhs_counts.items(), key=count_key)

        lhs_values = tuple(dataset.iloc[cluster_row_indices[0], lhs_indices])
        metric = number_metric if pandas.api.types.is_numeric_dtype(dataset.iloc[:, rhs_index]) \
            else string_metric
        filtered_cluster = filter_radius(lhs_values, sorted_rhs_counts, metric)

        if filtered_cluster:
            squashed_clusters.append(filtered_cluster)

    return squashed_clusters


def number_metric(a, b):
    return abs(a - b)


def string_metric(a, b):
    return levenshtein_distance(str(a), str(b))


def filter_radius(lhs_values, sorted_rhs_counts, metric):
    def is_value_close(value_count_pair):
        value, _ = value_count_pair
        return metric(most_common_value, value) < RADIUS

    most_common_value, _ = sorted_rhs_counts[0]
    close_value_pairs = list(filter(is_value_close, islice(sorted_rhs_counts, 1, None)))
    if close_value_pairs:
        return (lhs_values, [sorted_rhs_counts[0]] + close_value_pairs)
    
    return tuple()


def get_result_set(df, algo_type, algo_config):
    algo = algo_type()
    algo.load_data(table=df, **algo_config)
    algo.execute(**algo_config)
    return set(algo.get_fds())


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
    print(Fore.RED + '\n'.join(islice(df_lines, 2, None)) + Style.RESET_ALL)
    print()


def get_typo_candidates_df(df, display_df):
    def get_mask(attr_info):
        col_name, value = attr_info
        return df[col_name] == value

    typo_candidate_rows = []
    typo_candidate_row_indices = []

    for _, row in display_df.iterrows():
        mask = reduce(operator.and_, map(get_mask, islice(row.items(), 1, None)))
        found_rows = df[mask]
        typo_candidate_rows.append(found_rows.values[0])
        typo_candidate_row_indices.append(found_rows.index.values[0])
    return pandas.DataFrame(typo_candidate_rows, columns=df.columns, index=typo_candidate_row_indices)


def main():
    configure_dataframe_print()

    print(CONFIG_STRING)
    print()

    df = pandas.read_csv(DATASET_PATH, sep=SEPARATOR, header=HEADER, na_filter=False)
    print('Dataset sample:')
    print(df)
    print()

    print('Searching for almost holding FDs...')
    print()
    holding_fds = get_result_set(df, EXACT_ALGORITHM_TYPE, EXACT_ALGO_CONFIG)
    close_fds = get_result_set(df, APPROXIMATE_ALGORITHM_TYPE, APPROXIMATE_ALGO_CONFIG)
    almost_holding_fds = sorted(close_fds - holding_fds, key=lambda fd: fd.to_index_tuple())
    print('Found! Almost holding FDs:')
    print('\n'.join(map(str, almost_holding_fds)))
    print()

    print(f'Selecting FD with index {FD_INDEX}:')
    lhs_indices = almost_holding_fds[FD_INDEX].lhs_indices
    rhs_index = almost_holding_fds[FD_INDEX].rhs_index
    squashed_sorted_clusters = get_squashed_sorted_filtered_clusters(df, lhs_indices, rhs_index)

    if not squashed_sorted_clusters:
        print('Nothing found. Try another FD or relax restrictions (radius, ratio, error).')
        return

    display_df = make_display_df(squashed_sorted_clusters, df, lhs_indices, rhs_index)
    print_display_df(display_df)
    print('Typo candidates and context:')
    print(get_typo_candidates_df(df, display_df).to_string())


if __name__ == '__main__':
    main()
