import desbordante

ERROR = 0.2
PER_TUPLE = 'per_tuple'
PER_VALUE = 'per_value'
TABLE = 'examples/datasets/glitchy_sensor_2.csv'


def print_results(pfd_verifier):
    error = pfd_verifier.get_error()
    if error <= ERROR:
        print('PFD holds')
    else:
        print(f'PFD with error {ERROR} does not hold')
        print(f'instead it holds with error {error}')
        print(f'Clusters violating PFD ({pfd_verifier.get_num_violating_clusters()}):')
        for cluster in pfd_verifier.get_violating_clusters():
            print(cluster)
    print()


# Loading input data
algo = desbordante.pfd_verification.algorithms.PFDVerifier()
algo.load_data(table=(TABLE, ',', True))

algo.execute(lhs_indices=[1], rhs_indices=[2], error=ERROR, error_measure=PER_VALUE)
print('Checking whether pFD [device_id] -> [data]')
print(f'with error {ERROR} and PerValue error measure holds:')
print_results(algo)

algo.execute(lhs_indices=[1], rhs_indices=[2], error=ERROR, error_measure=PER_TUPLE)
print('Checking whether the same PFD holds for PerTuple error measure:')
print_results(algo)
