import desbordante
import pandas as pd

ERROR = 0.3
PER_TUPLE = 'per_tuple'
PER_VALUE = 'per_value'
TABLE = 'examples/datasets/glitchy_sensor_2.csv'


def print_results(verifier):
    error = verifier.get_error()
    if error <= ERROR:
        print('PFD holds')
    else:
        print(f'PFD with error {ERROR} does not hold')
        print(f'But it holds with error {error}')
        print()
        print('Additional info:')
        print(f'Number of rows violating PFD: {verifier.get_num_violating_rows()}')
        print(f'Number of clusters violating PFD: {verifier.get_num_violating_clusters()}')
        print()

        table = pd.read_csv(TABLE)
        violating_clusters = verifier.get_violating_clusters()
        number_names = ['First', 'Second', 'Third']
        cluster_number = 0
        for violating_cluster in violating_clusters:
            print(f'{number_names[cluster_number]} violating cluster:')
            cluster_number += 1
            violating_series = []
            for i, row in table.iterrows():
                if i not in violating_cluster:
                    continue
                violating_series.append(row)
            print(pd.DataFrame(violating_series))
            print()

# Loading input data
algo = desbordante.pfd_verification.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))

# Print dataset
print(f'Dataset: {TABLE}')
print(pd.read_csv(TABLE))
print()

# Checking whether PFD (DeviceId) -> (Data) holds for PerValue measure
algo.execute(lhs_indices=[1], rhs_indices=[2], error=ERROR, pfd_error_measure=PER_VALUE)
print('-' * 80)
print(f'Checking whether PFD (DeviceId) -> (Data) holds for {PER_VALUE} error measure')
print('-' * 80)
print_results(algo)

# Checking whether the same PFD holds for PerTuple measure
algo.execute(lhs_indices=[1], rhs_indices=[2], error=ERROR, pfd_error_measure=PER_TUPLE)
print('-' * 80)
print(f'Checking whether the same PFD holds for {PER_TUPLE} error measure:')
print('-' * 80)
print_results(algo)
