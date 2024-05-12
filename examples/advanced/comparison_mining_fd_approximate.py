import desbordante

print('''
=======================================================
This example show some work features of algorithms for
searching FD approximately: AidFD and EulerFD.
=======================================================\n
''')

SEED_76_FD = 2704
SEED_78_FD = 1321
SEED_80_FD = 9208

TABLE = 'examples/datasets/adult.csv'

alg = desbordante.fd.algorithms.EulerFD()
alg.load_data(table=(TABLE, ';', False))

print("EulerFD is randomized algorithm. Answer is depended on seed: ")

alg.execute(custom_random=(True, SEED_76_FD))
result76 = set(alg.get_fds())
print(f"Seed is {SEED_76_FD}, number of FDs is {len(result76)}")

alg.execute(custom_random=(True, SEED_78_FD))
result78 = set(alg.get_fds())
print(f"Seed is {SEED_78_FD}, number of FDs is {len(result78)}")

alg.execute(custom_random=(True, SEED_80_FD))
result80 = set(alg.get_fds())
print(f"Seed is {SEED_80_FD}, number of FDs is {len(result80)}")

exact_fd = desbordante.fd.algorithms.HyFD()
exact_fd.load_data(table=(TABLE, ';', False))
exact_fd.execute()
result_exact = set(exact_fd.get_fds())

print(f"Exact algorithm, number of FDs is {len(result_exact)}")

print("EulerFD can get extra answers (false FDs) or not find some FDs")

print()
print("---------------------------------------------------------------------")
print("Lets look at difference between answer of exact algorithm and EulerFD")
print()
print(f"Check first answer with seed {SEED_76_FD}, whick get as 76 FDs")

diff_76 = result_exact - result76
print(f"EulerFD not find {len(diff_76)} FD:")
for fd in diff_76:
    print(fd)

diff_76 = result76 - result_exact
print(f"EulerFD find {len(diff_76)} false FD:")
for fd in diff_76:
    print(fd)

print("EulerFD in one answer can not find some FDs and get false FDs instead")

print()
print(f"Lest check second answer with seed {SEED_78_FD}, which get as 78 FDs")
print(f"EulerFD not find {len(result_exact - result78)} FDs.")
print(f"EulerFD find {len(result78 - result_exact)} false FDs:")
print(f"EulerFD get exact answer with seed {SEED_78_FD}")

