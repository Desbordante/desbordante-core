import desbordante
import pandas as pd

def print_results(ucc_verifier):
    if ucc_verifier.ucc_holds():
        print('UCC holds, showing stats for AUCC is useless')
    else:
        print('UCC does not hold')
        print(f'But AUCC with error = {"{:.4f}".format(algo.get_error())} holds')
        print()
        print('Also:')
        print(f'Total number of rows violating UCC: {ucc_verifier.get_num_rows_violating_ucc()}')
        print(f'Number of clusters violating UCC: {ucc_verifier.get_num_clusters_violating_ucc()}')
        print('Clusters violating UCC:')
        print(f'found {ucc_verifier.get_num_clusters_violating_ucc()} clusters violating UCC:')
        table = pd.read_csv('examples/datasets/AUCC_example.csv')
        violating_clusters = ucc_verifier.get_clusters_violating_ucc()        

        cluster_number = 0
        number_names = ['First','Second','Third']
        print()
        for violating_cluster in violating_clusters:
            print(f"{number_names[cluster_number]} violating cluster:")
            cluster_number+=1
            violating_series = []
            for i, row in table.iterrows():
                if i not in violating_cluster:
                    continue
                violating_series.append(row)
            print(pd.DataFrame(violating_series))     
    print()


# Loading input data
algo = desbordante.aucc_verification.algorithms.Default()
algo.load_data(table=('examples/datasets/AUCC_example.csv', ',', True))

print("Dataset AUCC_example.csv:")
table = pd.read_csv('examples/datasets/AUCC_example.csv')
print(table)

# Checking whether (ID) UCC holds
algo.execute(ucc_indices=[0])
print('-'*80)
print('Checking whether (ID) UCC holds')
print('-'*80)
print()
print_results(algo)

# Checking whether (name) UCC holds
algo.execute(ucc_indices=[1])
print('-'*80)
print('Checking whether (name) UCC holds')
print('It should not hold, there are 2 persons, named Alex')
print('-'*80)
print()
print_results(algo)

# Checking whether (card_num) UCC holds
algo.execute(ucc_indices=[2])
print('-'*80)
print('Checking whether (card_num) UCC holds')
print('It should not hold, there are 2 identical card numbers')
print('-'*80)
print()
print_results(algo)

# Checking whether (card_num, card_active) UCC holds
algo.execute(ucc_indices=[2,3])
print('-'*80)
print('Checking whether (card_num, card_active) UCC holds')
print('It should hold, cards with identical numbers are not active simultaneously')
print('-'*80)
print()
print_results(algo)
print('-'*80)
