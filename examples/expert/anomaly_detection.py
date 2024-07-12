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
EXACT_ALGORITHM_TYPE = desbordante.fd.algorithms.Default
EXACT_ALGORITHM_CONFIG = {}

# Algorithm that finds approximate FDs and its config.
APPROXIMATE_ALGORITHM_TYPE = desbordante.afd.algorithms.Default
ERROR = 0.005
APPROXIMATE_ALGORITHM_CONFIG = {'error': ERROR}

METRIC_VERIFIER_CONFIG = {
    'lhs_indices': [1], 'rhs_indices': [3], 'metric': 'euclidean',
    'metric_algorithm': 'brute', 'parameter': 4
}


def get_result_fd(df, algo_type, algo_config):
    algo = algo_type()
    algo.load_data(table=df, **algo_config)
    algo.execute(**algo_config)
    return algo.get_fds()


def get_result_mv(df, mv_config):
    mv = desbordante.mfd_verification.algorithms.Default()
    mv.load_data(table=df, **mv_config)
    mv.execute(**mv_config)
    return mv.mfd_holds()


def print_fds(fds):
    print('\n'.join(map(str, sorted(fds, key=lambda fd: fd.to_name_tuple()))))


def main():
    df1 = pandas.read_csv(DATASET1_PATH, sep=SEPARATOR, header=HEADER)
    df2 = pandas.read_csv(DATASET2_PATH, sep=SEPARATOR, header=HEADER)
    df3 = pandas.read_csv(DATASET3_PATH, sep=SEPARATOR, header=HEADER)
    if not (df1.columns.tolist() == df2.columns.tolist() == df3.columns.tolist()):
        print('Datasets must have the same schemas!')
        return

    print("FDs found for dataset 1:")
    fds1 = get_result_fd(df1, EXACT_ALGORITHM_TYPE, EXACT_ALGORITHM_CONFIG)
    print_fds(fds1)

    print("FDs found for dataset 2:")
    fds2 = get_result_fd(df2, EXACT_ALGORITHM_TYPE, EXACT_ALGORITHM_CONFIG)
    print_fds(fds2)

    print("FDs found for dataset 3:")
    fds3 = get_result_fd(df3, EXACT_ALGORITHM_TYPE, EXACT_ALGORITHM_CONFIG)
    print_fds(fds3)

    print("AFDs found for dataset 3:")
    afds = get_result_fd(df3, APPROXIMATE_ALGORITHM_TYPE, APPROXIMATE_ALGORITHM_CONFIG)
    print_fds(afds)

    mfd_holds = get_result_mv(df3, METRIC_VERIFIER_CONFIG)
    print("MFD holds." if mfd_holds else "MFD does not hold.")


if __name__ == "__main__":
    main()
