import desbordante

print('''
=======================================================
This example show how Desbardante can seaching
functional dependencies (FD) approximately. There are 2
algorithms: EulerFD and AidFD, which can approximately
search FD, they may not find some FD or get false FD
unlike exact FD algorithms, but works by several
times faster.

For more information consider:
1) EulerFD: An Efficient Double-Cycle Approximation
   of Functional Dependencies by
   Qiongqiong Lin, Yunfan Gu, Jingyan Sa et al.
2) TODO: AidFD article
=======================================================\n
Now, we are going to demonstrate how to use EulerFD and AidFD.''')

TABLE = 'examples/datasets/medical.csv'

print("EulerFD: ")
alg = desbordante.fd.algorithms.EulerFD()
alg.load_data(table=(TABLE, ',', True))
alg.execute()

result_euler = alg.get_fds()

for fd in result_euler:
    print(fd)

print('-------------------------------')

print("AidFD: ")
alg = desbordante.fd.algorithms.Aid()
alg.load_data(table=(TABLE, ',', True))
alg.execute()

result_aid = alg.get_fds()

for fd in result_aid:
    print(fd)

print("Also there is more complex example in advanced part with more work features of algorithms")