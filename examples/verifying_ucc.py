import desbordante


def print_results(ucc_verifier):
    if ucc_verifier.ucc_holds():
        print('UCC holds')
    else:
        print('UCC does not hold')
        print(f'Total number of rows violating UCC: {ucc_verifier.get_num_rows_violating_ucc()}')
        print(f'Number of clusters violating UCC: {ucc_verifier.get_num_clusters_violating_ucc()}')
        print('Clusters violating UCC:')
        clusters_violating_ucc = ucc_verifier.get_clusters_violating_ucc()
        for cluster in clusters_violating_ucc:
            print(cluster)
    print()


# Loading input data
algo = desbordante.UCCVerifier()
algo.load_data('examples/datasets/actors_and_actress.csv', ',', True)

# Checking whether (First Name) UCC holds
algo.execute(ucc_indices=[1])
print('Checking whether (First Name) UCC holds')
print_results(algo)

# Checking whether (First Name, Last Name) UCC holds
algo.execute(ucc_indices=[1, 2])
print('Checking whether (First Name, Last Name) UCC holds')
print_results(algo)

# Checking whether (Born Town, Born Country) UCC holds
algo.execute(ucc_indices=[4, 5])
print('Checking whether (Born Town, Born Country) UCC holds')
print_results(algo)
