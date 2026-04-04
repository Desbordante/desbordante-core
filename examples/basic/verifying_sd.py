import sys
import os
import pandas as pd

import desbordante

TABLE_PATH = 'examples/datasets/sd_verification_datasets/network_logs.csv'
TABLE_PATH_FIXED = 'examples/datasets/sd_verification_datasets/network_logs_fixed.csv'

COLOR_CODES = {
    'bold_red': '\u001b[1;31m',
    'bold_green': '\033[1;32m',
    'default': '\033[0m',
    'default_bg': '\033[1;49m',
    'red_bg':'\033[1;41m',
    'green_bg':'\033[1;42m',
    'blue':'\033[1;34m',
    'yellow_bg': '\033[1;43m'
}

def print_sd_results(sd_verifier, lhs_name, rhs_name, g1, g2):
    confidence = sd_verifier.get_confidence()
    ops = sd_verifier.get_ops()
    holds = sd_verifier.holds(0.0) 
    
    print(f"{COLOR_CODES['default_bg']}SD: {lhs_name} -> [{g1}, {g2}] {rhs_name}")
    if holds:
        print(f"SD strictly holds: {COLOR_CODES['bold_green']}{holds}{COLOR_CODES['default']}")
    else:
        print(f"SD strictly holds: {COLOR_CODES['bold_red']}{holds}{COLOR_CODES['default']}")

    print(f"Operations needed (OPS): {ops}")
    print(f"Confidence: {confidence:.4f}")
    
    violations = sd_verifier.get_violations()
    if violations:
        print(f"\n{COLOR_CODES['blue']}--- Detected Violations ---{COLOR_CODES['default']}")
        for i, v in enumerate(violations, start=1):
            if isinstance(v, desbordante.sd_verification.SDDeletion):
                print(f"  {COLOR_CODES['bold_red']}#{i} DELETION:{COLOR_CODES['default']} Row index {v.row_idx} must be deleted.")
            elif isinstance(v, desbordante.sd_verification.SDInsertion):
                print(f"  {COLOR_CODES['yellow_bg']}#{i} INSERTION:{COLOR_CODES['default']} Gap between row {v.left_row_idx} and {v.right_row_idx}.")
                print(f"       Values: {v.val_left} -> {v.val_right}. Insertions: from {v.min_insertions} to {v.max_insertions}")
    print()

def auto_fix_dataset(table, violations):
    deleted_indices =[v.row_idx for v in violations if isinstance(v, desbordante.sd_verification.SDDeletion)]
    table_fixed = table.drop(index=deleted_indices)
    
    new_rows =[]
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
    print("\nIn this example, let's look at a dataset containing network performance statistics, specifically network polls probed periodically.\n")
    print(table)
    
    print("\nLet's say we want to check whether the data collector probes the network routers at the expected frequency (e.g., every 9 to 11 seconds).\n")
    
    print('This hypothesis will be expressed as a Sequential Dependency (SD) rule: PollNum -> [9.0, 11.0] Time\n')

    print("--- Original Data Results ---")
    print_sd_results(algo, "PollNum", "Time", g1, g2)

    table_fixed = auto_fix_dataset(table, algo.get_violations())
    clean_fixed = table_fixed.drop(columns=['_is_fixed']) if '_is_fixed' in table_fixed.columns else table_fixed
    print("\nWe can see that the rule is violated in several places. "
          "This may indicate missing data due to an unresponsive router, or spurious measurements. "
          "Let's automatically fix the dataset by deleting extra records and inserting missing ones.\n")
    clean_fixed.to_csv(TABLE_PATH_FIXED, index=False)

    print(clean_fixed)
    print(f"--- Verification of Fixed Data (Saved to {TABLE_PATH_FIXED}) ---")
    algo_fixed = desbordante.sd_verification.algorithms.SDVerifier()
    algo_fixed.load_data(table=clean_fixed)
    algo_fixed.execute(g1=g1, g2=g2, lhs_indices=[0], rhs_indices=[1])
    print_sd_results(algo_fixed, "PollNum", "Time", g1, g2)
    print("Note: When inserting missing records to bridge the time gaps, we simply duplicate the 'PollNum' "
          "of the preceding valid record. This choice keeps the fix local and prevents "
          "the need to shift and rewrite all subsequent 'PollNum' values in the entire table.\n")

if __name__ == '__main__':
    print("This example demonstrates how to validate Sequential Dependencies (SDs) using the Desbordante library.\n"
      "An SD expresses a relationship between ordered attributes, "
      "written as X ->[g1, g2] Y.\n"
      "This means that when the dataset is sorted by X, the difference between the Y-values "
      "of any two consecutive records must fall within the specified interval[g1, g2].\n"
      "Validation checks whether a user-specified SD holds for a given dataset, utilizing an edit-distance based confidence metric.\n"
      "Confidence is determined by the minimum number of operations (OPS) — record insertions or deletions — "
      "required to make the sequence completely valid.\n"
      "Desbordante detects SD violations, pinpointing exactly which rows must be deleted "
      "and where virtual records should be inserted to restore the correct sequence.\n")
    scenario_network_monitoring()
