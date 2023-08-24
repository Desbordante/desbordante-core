from collections import defaultdict, deque

import desbordante as desb
import pandas as pd

ALGORITHM = 'Pyro'
ERROR = 0.001
CONFIG = {'error': ERROR, 'max_lhs': 1}
DATASET_PATH = 'datasets/duplicates.csv'
HEADER = 0
SEPARATOR = ','
OUTPUT_FILE = 'output.csv'
WINDOW_SIZE = 4

CONFIG_STRING = f"""Deduplication parameters:
{ALGORITHM=}
{ERROR=:.5f}
{DATASET_PATH=}
{SEPARATOR=}
{WINDOW_SIZE=}
"""


def get_1lhs_fds(df, algo_name, algo_config):
    algo = getattr(desb, algo_name)()
    algo.load_data(df, **algo_config)
    algo.execute(**algo_config)
    return sorted((lhs_indices[0], fd.rhs_index) for fd in algo.get_fds()
                  if len(lhs_indices := fd.lhs_indices) == 1)


def get_lhs_from_sorted_fds(fds):
    lhs = []
    prev_lhs = None
    for cur_lhs, _ in fds:
        if cur_lhs != prev_lhs:
            lhs.append(cur_lhs)
        prev_lhs = cur_lhs
    return lhs


def count_matches(row1, row2, rhs: list[int]):
    return sum(row1[index] == row2[index] for index in rhs)


def setup_pandas_print():
    pd.set_option('display.max_columns', None)
    pd.set_option('display.width', None)
    pd.set_option('display.max_colwidth', None)


def print_fd_info(df: pd.DataFrame, fds: list[tuple[int, int]]):
    fd_dict = defaultdict(list)
    for lhs, rhs in fds:
        fd_dict[lhs].append(df.columns[rhs])
    print('AFD info:')
    print('\n'.join(f'{lhs}: {df.columns[lhs]} -> ( {" ".join(fd_dict[lhs])} )'
                    for lhs in get_lhs_from_sorted_fds(fds)))


def keepall_handler(df, new_rows, remaining_rows, used_rows):
    new_rows.extend(df.iloc[list(remaining_rows)].itertuples(index=False))
    remaining_rows.clear()


def drop_handler(df, new_rows, remaining_rows, used_rows):
    indices_to_add = list(remaining_rows - used_rows)
    new_rows.extend(df.iloc[indices_to_add].itertuples(index=False))
    remaining_rows.clear()


def choose_index(col_name, distinct_values):
    print(f'Column: {col_name}. Which value to use?')
    print('\n'.join(f'{i}: {value}' for i, value in enumerate(distinct_values)))
    return int(input('index: '))


def merge_handler(df, new_rows, remaining_rows, used_rows):
    if not used_rows:
        return
    new_row = []
    for col_name, values in zip(df.columns,
                                zip(*df.iloc[list(used_rows)].itertuples(index=False))):
        distinct_values = list(set(values))
        index = 0 if len(distinct_values) == 1 else choose_index(col_name, distinct_values)
        new_row.append(distinct_values[index])
    remaining_rows -= used_rows
    new_rows.append(new_row)


def unknown_handler(df, new_rows, remaining_rows, used_rows):
    print('Unknown command.')


def ask_rows(df: pd.DataFrame, window: deque[tuple[int, object]]) -> list:
    commands = {
        'keepall': keepall_handler,
        'drop': drop_handler,
        'merge': merge_handler,
    }

    remaining_rows = {row_info[0] for row_info in window}
    new_rows = []
    while remaining_rows:
        print(df.iloc[sorted(remaining_rows)].to_string())
        command_args = input('Command: ').split()
        if not command_args:
            print('Please input a command!')
            continue
        command, *used_rows = command_args
        used_rows = {col_num for col in used_rows if (col_num := int(col)) in remaining_rows}
        commands.get(command, unknown_handler)(df, new_rows, remaining_rows, used_rows)
    return new_rows


def is_similar(row_info, window, chosen_cols, matches_required):
    return any(count_matches(prev_row_info[1], row_info[1], chosen_cols) >= matches_required
               for prev_row_info in window)


def get_deduped_rows(df: pd.DataFrame, chosen_cols: list[int], matches_required: int,
                     fds: list[tuple[int, int]]):
    df.sort_values([df.columns[rhs_col] for _, rhs_col in fds if rhs_col in chosen_cols],
                   inplace=True)
    df.reset_index(inplace=True, drop=True)

    window = deque()
    new_rows = []
    has_duplicate = False
    for row_info in df.iterrows():
        if len(window) < WINDOW_SIZE:
            if not has_duplicate:
                has_duplicate = is_similar(row_info, window, chosen_cols, matches_required)
        elif not has_duplicate:
            new_rows.append(window.pop()[1].values)
            has_duplicate = is_similar(row_info, window, chosen_cols, matches_required)
        elif not is_similar(row_info, window, chosen_cols, matches_required):
            new_rows.extend(ask_rows(df, window))
            window.clear()
            has_duplicate = False
        window.appendleft(row_info)
    new_rows.extend(
        ask_rows(df, window) if has_duplicate else (row_info[1].values for row_info in window))
    return new_rows


def main():
    setup_pandas_print()
    print(CONFIG_STRING)

    df = pd.read_csv(DATASET_PATH, sep=SEPARATOR, header=HEADER, dtype='string', index_col=False, na_filter=False)
    print('Dataset sample:')
    print(df)
    print(f'Original records: {len(df)}')
    print()

    fds = get_1lhs_fds(df, ALGORITHM, CONFIG)
    print_fd_info(df, fds)
    lhs_column = int(input('LHS column index: '))
    fds = list(filter(lambda fd: fd[0] == lhs_column, fds))
    if not fds:
        print('No FDs with this LHS!')
        return
    print('RHS columns:')
    print('\n'.join(f'{rhs}: {df.columns[rhs]}' for _, rhs in fds))
    chosen_cols = sorted(set(map(int, input('RHS columns to use (indices): ').split())))
    matches_required = int(input('Equal columns to consider duplicates: '))

    new_rows = get_deduped_rows(df, chosen_cols, matches_required, fds)
    print()

    print(f'Resulting records: {len(new_rows)}. Duplicates found: {len(df) - len(new_rows)}')
    new_df = pd.DataFrame(new_rows, columns=df.columns)

    print(new_df)
    new_df.to_csv(OUTPUT_FILE, index=False)


if __name__ == '__main__':
    main()
