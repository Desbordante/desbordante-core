import desbordante

print('''
=======================================================
This example demonstrates how Desbordante can perform
approximate functional dependency (FD) discovery
methods.
It utilizes two algorithms, EulerFD and AID-FD, which
offer significant speed advantages over exact
FD discovery methods. While these algorithms may not
identify all true FDs or might occasionally yield
false positives, they achieve substantially faster
processing times.

For more in-depth information, please refer
to the following publications:
1) EulerFD: An Efficient Double-Cycle Approximation
   of Functional Dependencies by
   Qiongqiong Lin, Yunfan Gu, Jingyan Sa et al.
2) Approximate Discovery of Functional Dependencies
   for Large Datasets by Tobias Bleifuss,
   Susanne Bulow, Johannes Frohnhofen et al.
=======================================================\n
We will now demonstrate how to invoke EulerFD and
AID-FD in Desbordante.
''')

TABLE = 'examples/datasets/medical.csv'

print("EulerFD: ")
alg = desbordante.fd.algorithms.EulerFD()
alg.load_data(table=(TABLE, ',', True))
alg.execute()

result_euler = alg.get_fds()

for fd in result_euler:
    print(fd)

print('-------------------------------')

print("AID-FD: ")
alg = desbordante.fd.algorithms.Aid()
alg.load_data(table=(TABLE, ',', True))
alg.execute()

result_aid = alg.get_fds()

for fd in result_aid:
    print(fd)

print("In the advanced section, a more complex example will showcase additional features of the algorithms.")
