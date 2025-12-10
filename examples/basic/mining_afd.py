import desbordante
import pandas as pd
from tabulate import tabulate

TABLE = 'examples/datasets/inventory_afd.csv'
ERROR = 0.3

print('''==============================================
In Desbordante we consider an approximate functional dependency (AFD)
any kind of functional dependency (FD) that employs an error metric and is not named
(e.g. soft functional dependencies). This metric is used to calculate the extent of
violation for a given exact FD and lies within [0, 1] range (the lower, the less violations
are found in data). For the discovery task a user can specify the threshold and Desbordante
will find all AFDs, which have their error equal or less than the threshold, according to the selected metric.

Currently, Desbordante supports:
1) Five metrics: g1, pdep, tau, mu+, rho.
2) Two algorithms for discovery of AFDs: Tane and Pyro, with Pyro being the fastest. 
Unfortunately, Pyro can handle only the g1 metric, for the rest use Tane.

For more information consider:
1) Measuring Approximate Functional Dependencies: A Comparative Study by M. Parciak et al.
2) Efficient Discovery of Approximate Dependencies by S. Kruse and F. Naumann.
3) TANE: An Efficient Algorithm for Discovering Functional and Approximate Dependencies by Y. Huhtala et al.
==============================================\n
Now, we are going to demonstrate how to discover AFDs. First, consider the dataset:''')

df = pd.read_csv(TABLE)
print(tabulate(df, headers=['Id','ProductName','Price'], showindex=False, tablefmt='psql')+'\n')


pyro_alg = desbordante.afd.algorithms.Default()
pyro_alg.load_data(table=(TABLE, ',', True))
pyro_alg.execute(error=ERROR)

result = pyro_alg.get_fds()

print('AFDs mined by Pyro with g1 measure:')
for fd in result:
    print(fd)
print()

ERROR_MEASURES = ['g1','pdep','tau','mu_plus', 'rho']

tane_alg = desbordante.afd.algorithms.Tane()
tane_alg.load_data(table=(TABLE, ',', True))
print("AFDs mined by Tane")
for MEASURE in  ERROR_MEASURES:
    tane_alg.execute(error = ERROR, afd_error_measure = MEASURE)
    result = tane_alg.get_fds()
    print(MEASURE+':')
    for fd in result:
        print(fd)
    print()

