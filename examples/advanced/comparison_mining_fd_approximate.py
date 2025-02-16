import desbordante

print('''
=======================================================
This example demonstrates key characteristics of the
approximate functional dependency (FD) discovery
algorithms, AID-FD and EulerFD.
=======================================================\n
''')

SEED_76_FD = 2704
SEED_78_FD = 1321
SEED_80_FD = 9208

TABLE = 'examples/datasets/adult.csv'

alg = desbordante.fd.algorithms.EulerFD()
alg.load_data(table=(TABLE, ';', False))

print("EulerFD is a randomized algorithm, and its results vary based on the seed value. For instance:")

alg.execute(custom_random_seed=SEED_76_FD)
result76 = set(alg.get_fds())
print(f"With a seed of {SEED_76_FD}, EulerFD found {len(result76)} FDs.")

alg.execute(custom_random_seed=SEED_78_FD)
result78 = set(alg.get_fds())
print(f"With a seed of {SEED_78_FD}, EulerFD found {len(result78)} FDs.")

alg.execute(custom_random_seed=SEED_80_FD)
result80 = set(alg.get_fds())
print(f"With a seed of {SEED_80_FD}, EulerFD found {len(result80)} FDs.")

exact_fd = desbordante.fd.algorithms.HyFD()
exact_fd.load_data(table=(TABLE, ';', False))
exact_fd.execute()
result_exact = set(exact_fd.get_fds())

print(f"An exact FD discovery algorithm, in contrast, consistently identified 78 FDs.")

print('''
This highlights a key property of EulerFD: it may produce results with both
false positives (extra FDs) and false negatives (missing FDs)
compared to exact methods.
''')

print()
print("---------------------------------------------------------------------")
print("Let's examine the differences between the results of the exact algorithm and EulerFD.")
print()
print(f"First, consider the results with a seed of {SEED_76_FD}, where EulerFD identified {len(result76)} FDs.")

diff_76 = result_exact - result76
print(f"Compared to the exact method, EulerFD failed to identify the following {len(diff_76)} FDs:")
for fd in diff_76:
    print(fd)

diff_76 = result76 - result_exact
print(f"Additionally, it incorrectly identified these {len(diff_76)} false FDs:")
for fd in diff_76:
    print(fd)

print("Thus, a single run of EulerFD can both miss valid FDs and generate false FDs.")

print()
print(f"Next, let's analyze the results with a seed of {SEED_78_FD}, where EulerFD identified 78 FDs") 
print(f"EulerFD not found {len(result_exact - result78)} FDs.")
print(f"EulerFD found {len(result78 - result_exact)} false FDs.")
print(f"Therefore, with the seed {SEED_78_FD}, EulerFD obtained the exact result.")

