import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

def prints(str):
    print(textwrap.fill(str, 80))

def print_data_frame(data_frame, title = None):
    print_table(data_frame, 'keys', title)

def print_table(table, headers = None, title = None):
    if title is not None:
        print(title)

    print(tabulate(table, headers=headers, tablefmt='psql'))


table = pd.read_csv('examples/datasets/salary.csv')
algo = desbordante.aod_verification.algorithms.Default()
algo.load_data(table=table)

prints("This example verifies set-based ODs.")
prints("""Please take a look at set-based ODs mining example first
(examples/basic/mining_set_od1.py).""")
print()
print_data_frame(table)
print()
prints("Let's start by verifying exact OC holding on the table above.")
prints("""One example of such OC is `{1} : 2<= ~ 3<=` (if you don't understand why it
holds, please take a look at examples/basic/mining_set_od1.py).""")
print()

# Indices are zero-based, this is why we're subtracting one
algo.execute(oc_context=[0], oc_left_index=1, oc_right_index=2, left_ordering='ascending')
prints(f"""OC {{1}}: 2<= ~ 3<= holds exactly: {algo.holds()}, removal set: {algo.get_removal_set()},
error: {algo.get_error()}""")
prints("""Note that error is zero and removal set is empty. Removal set is a set of rows which
should be removed in order for OC (or OD) to holds exactly. In this case OC holds exactly and
that's why the set is empty.""")

print()

prints("Now let's verify OFD {2} : [] -> 1<= which also holds exactly.")
print()
algo.execute(ofd_context=[1], ofd_right_index=0)
prints(f"""OFD {{2}}: [] -> 1<= holds exactly: {algo.holds()}, removal set: {algo.get_removal_set()},
error: {algo.get_error()}""")
prints("Note once again that error is zero and removal set is empty because OFD holds exactly")

print()
print("Now let's add some lines to the table to break exact holding of dependencies.")
table.loc[8] = [2020, 50, 9000]
print_data_frame(table)

# Need to recreate algo object since currently calling load_data() twice is not supported yet
algo = desbordante.aod_verification.algorithms.Default()
algo.load_data(table=table)
algo.execute(oc_context=[0], oc_left_index=1, oc_right_index=2, left_ordering='ascending')
prints(f"""OC {{1}}: 2<= ~ 3<= holds exactly: {algo.holds()}, removal set: {algo.get_removal_set()},
error: {algo.get_error()}""")
prints("""Note that now OC doesn't hold exactly and that removal set is {4}. This means that
in order for OC to hold exactly, it's enough to remove from the table line number 4 (indexed from 0).
Note that lines 8 and 4 are interchangable in that sense, because the problem with ordering is
caused by their simultaneous presence in the table and removing any of them will fix it. Algorithm
guarantees to return a minimal removal set in terms of size, but doesn't specify which one exactly
if there are several possible.""")

print()
algo.execute(ofd_context=[1], ofd_right_index=0)
prints(f"""OFD {{2}}: [] -> 1<= holds exactly: {algo.holds()}, removal set: {algo.get_removal_set()},
error: {algo.get_error()}""")
prints("""Note once again that the OFD does not hold exactly anymore and that removal set is not
empty. By adding line 8 with the same value in column 2 as in line 5, but different values in column
1 we broke FD 2->1 and thus broke OFD {2}: [] -> 1<=. Removing any of these two lines will make the
OFD hold exactly, thus removal set is {5}.
""")

