import sys
import os
import pandas as pd

import desbordante


TABLE_PATH = (
    'examples/datasets/sd_verification_datasets/network_logs.csv'
)
TABLE_PATH_FIXED = (
    'examples/datasets/sd_verification_datasets/network_logs_fixed.csv'
)

COLOR_CODES = {
    'bold_red': '\u001b[1;31m',
    'bold_green': '\033[1;32m',
    'default': '\033[0m',
    'default_bg': '\033[1;49m',
    'red_bg': '\033[1;41m',
    'green_bg': '\033[1;42m',
    'blue': '\033[1;34m',
    'yellow_bg': '\033[1;43m'
}


def print_sd_results(sd_verifier, lhs_name, rhs_name, g1, g2):
    confidence = sd_verifier.get_confidence()
    ops = sd_verifier.get_ops()
    holds = sd_verifier.holds(0.0)

    print(
        f"{COLOR_CODES['default_bg']}SD: {lhs_name} -> [{g1}, {g2}] "
        f"{rhs_name}"
    )
    if holds:
        print(f"SD strictly holds: {COLOR_CODES['bold_green']}{holds}"
              f"{COLOR_CODES['default']}")
    else:
        print(f"SD strictly holds: {COLOR_CODES['bold_red']}{holds}"
              f"{COLOR_CODES['default']}")

    print(f"Operations needed (OPS): {ops}")
    print(f"Confidence: {confidence:.4f}")

    violations = sd_verifier.get_violations()
    if violations:
        print(f"\n{COLOR_CODES['blue']}--- Detected Violations ---"
              f"{COLOR_CODES['default']}")
        for i, v in enumerate(violations, start=1):
            if isinstance(v, desbordante.sd_verification.SDDeletion):
                print(f"  {COLOR_CODES['bold_red']}#{i} DELETION:"
                      f"{COLOR_CODES['default']} "
                      f"Row index {v.row_idx} must be deleted.")
            elif isinstance(v, desbordante.sd_verification.SDInsertion):
                print(f"  {COLOR_CODES['yellow_bg']}#{i} INSERTION:"
                      f"{COLOR_CODES['default']} "
                      f"Gap between row {v.left_row_idx} and "
                      f"{v.right_row_idx}.")
                print(
                    f"       Values: {v.val_left} -> {v.val_right}. "
                    f"Number of required insertions: from "
                    f"{v.min_insertions} to {v.max_insertions}"
                )
    print()


def auto_fix_dataset(table, violations):
    deleted_indices = [
        v.row_idx for v in violations
        if isinstance(v, desbordante.sd_verification.SDDeletion)
    ]
    table_fixed = table.drop(index=deleted_indices)

    new_rows = []
    for v in violations:
        if isinstance(v, desbordante.sd_verification.SDInsertion):
            num_ins = v.min_insertions
            time_step = (v.val_right - v.val_left) / (num_ins + 1)
            poll_left = table.loc[v.left_row_idx, 'PollNum']
            poll_right = table.loc[v.right_row_idx, 'PollNum']

            for k in range(1, num_ins + 1):
                new_rows.append({
                    'PollNum': poll_left,
                    'Time': int(v.val_left + time_step * k)
                })

    if new_rows:
        df_new = pd.DataFrame(new_rows)
        df_new['_is_fixed'] = True
        table_fixed['_is_fixed'] = False
        table_fixed = pd.concat([table_fixed, df_new], ignore_index=True)

    return table_fixed.sort_values('Time').reset_index(drop=True)


def scenario_network_monitoring():
    if not os.path.exists(TABLE_PATH):
        print(f"Error: Dataset {TABLE_PATH} not found.")
        return

    table = pd.read_csv(TABLE_PATH)
    algo = desbordante.sd_verification.algorithms.SDVerifier()

    algo.load_data(table=table)

    g1, g2 = 9.0, 11.0
    algo.execute(g1=g1, g2=g2, lhs_indices=[0], rhs_indices=[1])
    print("\nIn this example, let's look at a dataset containing network "
          "performance statistics.")
    print("Specifically, network polls probed periodically.")
    print()

    print("\nLet's say we want to check whether the data collector "
          "probes the network")
    print("routers at the expected frequency.")
    print("(e.g., every 9 to 11 seconds).")
    print()

    print(table)
    print("--- Original Data Results ---")
    print_sd_results(algo, "PollNum", "Time", g1, g2)

    table_fixed = auto_fix_dataset(table, algo.get_violations())
    cols = table_fixed.columns.tolist()
    clean_fixed = (
        table_fixed.drop(columns=['_is_fixed'])
        if '_is_fixed' in cols
        else table_fixed
    )
    print("\nWe can see that the rule is violated in several places.\n"
          "This may indicate missing data due to an unresponsive router,\n"
          "or spurious measurements.")
    print("Let's automatically fix the dataset by deleting extra records "
          "and inserting missing ones.")
    clean_fixed.to_csv(TABLE_PATH_FIXED, index=False)

    print(clean_fixed)
    print(f"Saved to {TABLE_PATH_FIXED}")
    print("--- Verification of Fixed Data ---")
    algo_fixed = desbordante.sd_verification.algorithms.SDVerifier()
    algo_fixed.load_data(table=clean_fixed)
    algo_fixed.execute(g1=g1, g2=g2, lhs_indices=[0], rhs_indices=[1])
    print_sd_results(algo_fixed, "PollNum", "Time", g1, g2)
    print("Note: When inserting missing records to bridge the time gaps,\n"
          "we simply duplicate the 'PollNum' of the preceding valid\n"
          "record.")
    print("This choice keeps the fix local and prevents the need to shift\n"
          "and rewrite all subsequent 'PollNum' values in the entire\n"
          "table.")


if __name__ == '__main__':
    print("This example demonstrates how to validate Sequential Dependencies\n"
          "(SDs) using the Desbordante library.")
    print("Algorithm is based on the article 'Sequential Dependencies',\n"
          "by Lukasz Golab, Howard Karloff, Flip Korn, Avishek Saha.")
    print()
    print("An SD expresses a relationship between ordered attributes,\n"
          "written as X -> [g1, g2] Y.")
    print("This means that when the dataset is sorted by X, the difference\n"
          "between the Y-values of any two consecutive records must fall\n"
          "within the specified interval [g1, g2].")
    print()
    print("Validation checks whether a user-specified SD holds for a given\n"
          "dataset, utilizing an edit-distance based confidence metric.")
    print("Confidence is determined by the minimum number of operations\n"
          "(OPS) — record insertions or deletions — required to make the\n"
          "sequence completely valid.")
    print()
    print("Confidence = (N - OPS) / N, where N is the number of rows in the\n"
          "dataset. If confidence equals 1, there is no exception to\n"
          "the dependency.")
    print("Confidence can't be exactly 0, because in the worst case we\n"
          "need to delete all but one record so the SD holds; thus,\n"
          "confidence is at least 1/N.")
    print()
    print("Desbordante detects SD violations, pinpointing exactly which\n"
          "rows must be deleted and where virtual records should be\n"
          "inserted to restore the correct sequence.")
    print("Right now, X and Y can be represented by a single column each;\n"
          "however, it can be expanded in the future.")
    scenario_network_monitoring()
