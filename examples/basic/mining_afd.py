import desbordante

TABLE = 'examples/datasets/inventory_afd.csv'
ERROR = 0.1
pyro_alg = desbordante.afd.algorithms.Default()
pyro_alg.load_data(table=(TABLE, ',', True))
pyro_alg.execute(error=ERROR)
result = pyro_alg.get_fds()

print('AFDs mined by pyro with g1 measure:')
for fd in result:
    print(fd)

print()
table=(TABLE, ',', True)
ERROR_MEASURES = ['g1','pdep','tau','mu_plus', 'rho']
print("AFDs mined by Tane")
for MEASURE in  ERROR_MEASURES:
    tane_alg = desbordante.afd.algorithms.Tane()
    tane_alg.load_data(table=(TABLE, ',', True))
    tane_alg.execute(error = ERROR, afd_error_measure = MEASURE)
    result = tane_alg.get_fds()
    print(MEASURE+':')
    for fd in result:
        print(fd)
    print()