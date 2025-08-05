import desbordante
import pandas as pd


RED_CODE = "\033[1;41m"
GREEN_CODE = "\033[1;42m"
BLUE_CODE = "\033[1;46m"
DELETE_CODE = "\033[1;31m"
INSERT_CODE = "\033[1;32m"
UPDATE_CODE = "\033[1;33m"
BOLD_CODE = "\033[1;49m"
DEFAULT_COLOR_CODE = "\033[0m"

max_data_index = 0

def print_clusters(verifier, data, lhs, rhs):
    print(f"Number of clusters violating FD: {verifier.get_num_error_clusters()}")
    lhs_columns = [data[data.columns[idx]] for idx in lhs]
    rhs_columns = [data[data.columns[idx]] for idx in rhs]
    for i, highlight in enumerate(verifier.get_highlights(), start=1):
        print(f"{BLUE_CODE} #{i} cluster: {DEFAULT_COLOR_CODE}")
        for el in highlight.cluster:
            lhs_values = [col[el] for col in lhs_columns]
            rhs_values = [col[el] for col in rhs_columns]
            print(f"{el}: {lhs_values} -> {rhs_values}")
        print(f"Most frequent rhs value proportion: {highlight.most_frequent_rhs_value_proportion}")
        print(f"Num distinct rhs values: {highlight.num_distinct_rhs_values}")

def print_results_for_afd(verifier, error):
    if verifier.get_error() < error:
        print(GREEN_CODE, "AFD with this error threshold holds", DEFAULT_COLOR_CODE)
    else:
        print(RED_CODE, "AFD with this error threshold does not hold", DEFAULT_COLOR_CODE)
        print(f"But the same {GREEN_CODE} AFD with error threshold = {verifier.get_error()} holds {DEFAULT_COLOR_CODE}")

def print_table_and_changes(data, insert_df=None, delete_set=None, update_df=None):
    insert_indices = []
    update_indices = []

    if insert_df is not None:
        insert_indices = range(max_data_index - len(insert_df) + 1, max_data_index + 1)
        print(INSERT_CODE + 'Rows to insert:')
        print(insert_df.to_string(index=False), end=DEFAULT_COLOR_CODE+'\n')
    if delete_set is not None:
        print(DELETE_CODE + 'Rows indices to delete: ', ', '.join([str(i) for i in delete_set]) + DEFAULT_COLOR_CODE)
    if update_df is not None:
        update_indices = set(update_df.index)
        print(UPDATE_CODE + 'Rows to update:')
        update_rows = update_df.to_string(index=True, index_names=False).split('\n')
        print('\n'.join(update_rows), end=DEFAULT_COLOR_CODE+'\n')
    
    print('\nCurrent table state:')
    data_rows = data.to_string().split('\n')
    print(data_rows[0])
    for string_row, (row_index, *_) in zip(data_rows[1:], data.itertuples()):
        style_code = UPDATE_CODE if row_index in update_indices else INSERT_CODE if row_index in insert_indices else BOLD_CODE
        print(style_code, end='')
        print(string_row, end='')
        print(DEFAULT_COLOR_CODE)

def update_and_print_data_table(data, insert_df=None, delete_set=None, update_df=None):
    global max_data_index
    # insert
    if insert_df is not None:
        insert_df.set_index(pd.Index(range(max_data_index + 1, max_data_index + len(insert_df) + 1)), inplace=True)
        data = pd.concat([data, insert_df])
        max_data_index = data.index.max()
    # delete
    if delete_set is not None:
        data = data.drop(list(delete_set))
    # update
    if update_df is not None:
        df = update_df.drop(['_id'], axis=1)
        df.set_index(update_df['_id'], inplace=True)
        data.update(df)
        update_df = df

    # printing with highlights
    print_table_and_changes(data, insert_df, delete_set, update_df)
    return data

def main_scenario(table='examples/datasets/DnD.csv'):
    global max_data_index
    print(BOLD_CODE + 'This example shows how to verify AFD in the dynamic table using dynamic_fd_verification.\n' + 
          'If you want to know how to use dynamic_fd_verification, please see dynamic_verifying_fd.py example.')
    print(f"{BOLD_CODE}First, let's look at the DnD.csv table and try verifying the [Creature, HaveMagic] -> [Strength] FD.{DEFAULT_COLOR_CODE}")
    data = pd.read_csv(table, header=[0])
    max_data_index = data.index.max()
    print_table_and_changes(data)
    algo = desbordante.dynamic_fd_verification.algorithms.Default()
    algo.load_data(table=data, lhs_indices=[0, 2], rhs_indices=[1])

    print("\nChecking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.5)")
    print_results_for_afd(algo, 0.5)
    print("\nChecking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.1)")
    print_results_for_afd(algo, 0.1)
    print("\nSimilarly to the FD verification primitive, the AFD one can provide a user with clusters:")
    print_clusters(algo, data, [0, 2], [1])

    print(f"{BOLD_CODE}Now, let's change the table and check the AFD again.{DEFAULT_COLOR_CODE}")
    insert_df = pd.DataFrame({'Creature': ['Elf', 'Elf', 'Dragon'], 'Strength': [2, 2, 200], 'HaveMagic': [False, False, True]})
    delete_set = {2, 4, 6}
    update_df = pd.DataFrame({'_id': [0, 1, 3, 5], 'Creature': ['Elf', 'Elf', 'Dragon', 'Elf'], 'Strength': [3, 3, 200, 3], 'HaveMagic': [False, False, True, False]})
    algo.execute(insert=insert_df, delete=delete_set, update=update_df)
    data = update_and_print_data_table(data=data, insert_df=insert_df, delete_set=delete_set, update_df=update_df)

    print("\nChecking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.5)")
    print_results_for_afd(algo, 0.5)
    print("\nChecking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.1)")
    print_results_for_afd(algo, 0.1)
    print("\nLet's look at the clusters again")
    print_clusters(algo, data, [0, 2], [1])

main_scenario()
