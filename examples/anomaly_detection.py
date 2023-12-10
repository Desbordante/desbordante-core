import desbordante
import pandas

from os.path import join

# Parameters for pandas.read_csv(...).
HEADER = 0
SEPARATOR = ','
DATASET_FOLDER_PATH = 'examples/datasets'
DATASET1_PATH = join(DATASET_FOLDER_PATH, "cargo_data_1.csv")
DATASET2_PATH = join(DATASET_FOLDER_PATH, "cargo_data_2.csv")
DATASET3_PATH = join(DATASET_FOLDER_PATH, "cargo_data_3.csv")

# Algorithm that finds exact FDs and its config.
EXACT_ALGORITHM_TYPE = desbordante.HyFD
EXACT_ALGORITHM_CONFIG = {}

# Algorithm that finds approximate FDs and its config.
APPROXIMATE_ALGORITHM_TYPE = desbordante.Pyro
ERROR = 0.005
APPROXIMATE_ALGORITHM_CONFIG = {'error': ERROR}

METRIC_VERIFIER_CONFIG = {
    'lhs_indices': [1], 'rhs_indices': [3], 'metric': 'euclidean',
    'metric_algorithm': 'brute', 'parameter': 4
}


def get_result_fd(df, algo_type, algo_config):
    algo = algo_type()
    algo.load_data(df, **algo_config)
    algo.execute(**algo_config)
    return {(tuple(fd.lhs_indices), fd.rhs_index) for fd in algo.get_fds()}


def get_result_mv(df, mv_config):
    mv = desbordante.MetricVerifier()
    mv.load_data(df, **mv_config)
    mv.execute(**mv_config)
    return mv.mfd_holds()


def get_named_col_fds(fds, columns):
    return sorted(
        ([columns[lhs_index] for lhs_index in lhs_indices],
         columns[rhs_index])
        for lhs_indices, rhs_index in fds
    )


def print_fds(fds, columns):
    for lhs_names, rhs_name in get_named_col_fds(fds, columns):
        print(", ".join(lhs_names), " -> ", rhs_name)


def main():
    df1 = pandas.read_csv(DATASET1_PATH, sep=SEPARATOR, header=HEADER)
    df2 = pandas.read_csv(DATASET2_PATH, sep=SEPARATOR, header=HEADER)
    df3 = pandas.read_csv(DATASET3_PATH, sep=SEPARATOR, header=HEADER)
    if not (df1.columns.tolist() == df2.columns.tolist() == df3.columns.tolist()):
        print('Datasets must have the same schemas!')
        return

    print("FDs found for dataset 1:")
    fds1 = get_result_fd(df1, EXACT_ALGORITHM_TYPE, EXACT_ALGORITHM_CONFIG)
    print_fds(fds1, df1.columns)

    print("FDs found for dataset 2:")
    fds2 = get_result_fd(df2, EXACT_ALGORITHM_TYPE, EXACT_ALGORITHM_CONFIG)
    print_fds(fds2, df1.columns)

    print("FDs found for dataset 3:")
    fds3 = get_result_fd(df3, EXACT_ALGORITHM_TYPE, EXACT_ALGORITHM_CONFIG)
    print_fds(fds3, df1.columns)

    print("AFDs found for dataset 3:")
    afds = get_result_fd(df3, APPROXIMATE_ALGORITHM_TYPE, APPROXIMATE_ALGORITHM_CONFIG)
    print_fds(afds, df1.columns)

    mfd_holds = get_result_mv(df3, METRIC_VERIFIER_CONFIG)
    print("MFD holds." if mfd_holds else "MFD does not hold.")


if __name__ == "__main__":
    main()
