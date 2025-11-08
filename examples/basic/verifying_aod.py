import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

# Auxiliary print functions


def prints(s):
    print(textwrap.fill(s, 80))


def printlns(s):
    prints(s)
    print()


def print_data_frame(data_frame, title=None):
    print_table(data_frame, 'keys', title)


def println_data_frame(data_frame, title=None):
    print_data_frame(data_frame, title)
    print()


def print_table(table, headers=None, title=None):
    if title is not None:
        print(title)

    print(tabulate(table, headers=headers, tablefmt='psql'))


def println_table(table, headers=None, title=None):
    print_table(table, headers, title)
    print()


def print_verification_result(algo, od):
    prints(
        f"""Running `algo.execute()` on {od} produces the following results:""")
    prints(f"    `algo.holds()`: {algo.holds()}")
    prints(f"    `algo.get_removal_set()`: {algo.get_removal_set()}")
    prints(f"    `algo.get_error()`: {algo.get_error()}")


# Actual verification logic
table = pd.read_csv('examples/datasets/salary.csv')
algo = desbordante.aod_verification.algorithms.Default()
algo.load_data(table=table)

prints("This example verifies set-based Order Dependencies (ODs).")
prints("""Set-based ODs were first introduced in the paper [1] Jaroslaw Szlichta, Parke Godfrey,
Lukasz Golab, Mehdi Kargar, and Divesh Srivastava. 2017. Effective and complete discovery of order
dependencies via set-based axiomatization. Proc. VLDB Endow. 10, 7 (March 2017), 721–732.
https://doi.org/10.14778/3067421.3067422""")
printlns("""This example is based on the subsequent work [2] Karegar, Reza et al. “Efficient
Discovery of Approximate Order Dependencies.” ArXiv abs/2101.02174 (2021): n. pag.""")

printlns("""First, let's introduce the required definitions from paper [2]. Note that there
is some discrepancy between definitions in [1] and [2]. In this example, we adhere to the
definitions and notations from [2].""")
printlns("""Definition 1. Let X and Y be lists of attributes in a table. X -> Y denotes an Order
Dependency. If t is a tuple, let t[X] denote a projection of t onto X. The dependency X -> Y holds
if and only if for any two tuples s and t, s[X] <= t[X] implies s[Y] <= t[Y].""")
printlns("""In simpler terms, if the dependency X->Y holds for a table r, it means that sorting the
table by the attribute list X also guarantees to sort it by the attribute list Y.""")


prints("Please observe the following dataset, which we are going to use throughout the example.")
println_data_frame(table)

printlns("""For example, in the table above, the order dependency ['year', 'employee_grade'] ->
['year', 'avg_salary'] holds, while the dependency ['year', 'employee_grade'] -> ['avg_salary'] does
not. If we sort the table by ['year', 'employee_grade'], it will also be guaranteed to be sorted by
['year', 'avg_salary'].""")

prints("""Definition 2. Let H be a set of attributes. We define an equivalence class of H as a
set of tuples where for any two tuples s and t in the set, s[H] = t[H].""")
printlns("""For example, in the table above, let's take H={'year'}. Then there are two equivalence
classes of H: one with the value 2020 (tuples with indices {0, 1, 2, 3, 4}) and the other with the
value 2021 (tuples with indices {5, 6, 7}).""")

printlns("""Definition 3. Given a set of attributes H and attributes A and B, H: A<= ~ B denotes a
Canonical Order Compatibility (OC) with relation <= for attribute A. The canonical order
compatibility H: A<= ~ B holds if and only if inside each equivalence class of H, there exists a
total ordering of tuples such that they are ordered by both A and B. A<= means that when we are
comparing s[A] vs t[A], we use the <= relation.""")
printlns("""For example, in the table above, the OC `{'year'} : 'employee_grade'<= ~ 'avg_salary'`
holds. Within both equivalence classes of {'year'}, a total ordering of tuples exists, so the
tuples are ordered by both 'employee_grade'<= and 'avg_salary'. That is, ordering [0,3,2,1,4] and
[6, 7, 5]. At the same time, the OC `{}: 'employee_grade'<= ~ 'avg_salary'` does not hold. For an
empty set of attributes, there is only one equivalence class, which consists of all tuples in the
table. There is no such ordering in the table that all tuples are sorted at the same time by
'employee_grade'<= and 'year'. This is because for tuples 2 and 7, the order for 'employee_grade'
is > while for 'avg_salary' it is <.""")

printlns("""Definition 4. Given a set of attributes H and an attribute A, H: [] -> A denotes a
Canonical Order Functional Dependency (OFD). H: [] -> A holds if and only if attribute A is
constant within each equivalence class of H. This is equivalent to the list-based OD H -> HA for
any permutation of H.""")
printlns("""For example, in the table above, the OFD {'avg_salary'}: [] -> ['employee_grade'] holds.
Since all values in the 'avg_salary' column are unique, each equivalence class of {'avg_salary'}
consists of exactly one tuple. Within each set of a single tuple, 'employee_grade' is constant (as
is any other attribute).""")

printlns("""Note that the list-based OD HA -> HB for any permutation of H is logically equivalent to
the OC H: A ~ B and the OFD HA: [] -> B. This means that the OD HA -> HB holds if and only if both
the corresponding OC and OFD hold.""")

printlns("""Definition 5. A set-based canonical order dependency denotes either a canonical order
compatibility or a canonical order functional dependency.""")

printlns("""Definition 6. An Approximate set-based canonical Order Dependency (AOD) is a set-based
canonical order dependency that holds only on a subset of the table. An AOD holds if and only if
there exists a set of tuples that can be removed from the table for the AOD to hold exactly. The
minimal set of such tuples is called a removal set. The error E of an AOD is calculated as the
cardinality of the removal set (the number of tuples in the set) divided by the cardinality of the
table. We say that an AOD holds with error E if and only if the error of the AOD is less than the
value E.""")

printlns("""You might also want to take a look at the set-based ODs mining example
(examples/basic/mining_set_od_1.py).""")

printlns("Now, let's move to the set-based canonical OD verification via Desbordante.")

prints("Let's start by verifying an exact OC that holds on the table above.")
printlns(
    """As we showed above, the OC `{'year'} : 'employee_grade'<= ~ 'avg_salary'` holds.""")

# Indices of columns are zero-based
algo.execute(oc_context=[0], oc_left_index=1,
             oc_right_index=2, left_ordering='ascending')
print_verification_result(
    algo, f"OC {{'year'}}: 'employee_grade'<= ~ 'avg_salary'")
printlns("""Note that the error is zero and the removal set is empty. A removal set is a set of
rows that should be removed for an OC (or OD) to hold exactly. In this case, the OC holds exactly,
which is why the set is empty.""")

printlns(
    "Now let's verify the OFD {'employee_grade'} : [] -> 'year', which also holds exactly.")
algo.execute(ofd_context=[1], ofd_right_index=0)
print_verification_result(algo, f"OFD {{'employee_grade'}}: [] -> 'year'")
printlns("""Note that the error once again is zero and the removal set is empty because the OFD holds
exactly.""")

prints("Now let's add a row to the table to break the exact holding of these dependencies.")
table.loc[8] = [2020, 50, 9000]
print_data_frame(table)
printlns("Note that the row with index 8 was added to the table.")

# Need to recreate the algo object since calling load_data() twice is not supported yet
algo = desbordante.aod_verification.algorithms.Default()
algo.load_data(table=table)
algo.execute(oc_context=[0], oc_left_index=1,
             oc_right_index=2, left_ordering='ascending')
print_verification_result(
    algo, f"OC {{'year'}}: 'employee_grade'<= ~ 'avg_salary'")
printlns("""Note that now the OC does not hold exactly and that the removal set is {4}. This means
that for the OC to hold exactly, it is enough to remove row number 4 (indexed from 0) from the
table. Note that rows 8 and 4 are interchangeable in this sense because the problem with ordering
is caused by their simultaneous presence in the table, and removing either of them will fix it. The
algorithm guarantees to return a minimal removal set in terms of size but does not specify which
one exactly if there are several candidates.""")

algo.execute(ofd_context=[1], ofd_right_index=0)
print_verification_result(algo, f"OFD {{'employee_grade'}}: [] -> 'year'")
printlns("""Note once again that the OFD does not hold exactly anymore and the removal set is not
empty. By adding row 8 with the same value in the 'employee_grade' column as in row 5 but with a
different value in the 'year' column, we broke the FD 'employee_grade'->'year' and thus broke the
OFD {'employee_grade'}: [] -> 'year'. Removing either of these two rows will make the OFD hold
exactly; thus, the removal set is {5}.""")

printlns("""We hope this example helped you understand how to verify exact and approximate set-based
order dependencies. We've seen how even a single row can affect the result and what a removal set is.
""")
prints("""Feel free to play around with the code: modify the table with different values, try
verifying other AODs, or even load your own datasets to see what you can discover!""")
