import desbordante
import pandas as pd
from tabulate import tabulate
import textwrap

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"
RESET_CODE = "\033[0m"

print('''==============================================
In Desbordante, we consider an approximate inclusion dependency (AIND)
as any inclusion dependency (IND) that utilizes an error metric to measure
violations. This metric calculates the proportion of distinct values in the
dependent set (LHS) that must be removed to satisfy the dependency on the
referenced set (RHS) completely.

The metric lies within the [0, 1] range:
- A value of 0 means the IND holds exactly (no violations exist).
- A value closer to 1 indicates a significant proportion of LHS values violate
the dependency.

Desbordante supports the discovery and verification of both exact INDs and AINDs:
1) Exact INDs: All values in the LHS set must match a value in the RHS set.
2) Approximate INDs (AINDs): Allows for controlled violations quantified by the
error metric.

For verification tasks, users can specify an AIND, and Desbordante will calculate
the error value, identifying clusters of violating values.

The error metric used for AINDs in Desbordante is adapted from the g3 metric,
initially developed for approximate functional dependencies (FDs).

For more information, consider:
1) Unary and n-ary inclusion dependency discovery in relational databases by
   Fabien De Marchi, Stéphane Lopes, and Jean-Marc Petit.

==============================================''')


def prints(str):
    print(textwrap.fill(str, 80))


def print_results_for_ind(verifier):
    if verifier.get_error() == 0:
        print(GREEN_CODE, "IND holds", RESET_CODE)
    else:
        print(RED_CODE, f"AIND holds with error = {verifier.get_error():.2}",
              RESET_CODE)


def print_table(table, title):
    print(title)
    print(tabulate(table, headers='keys', tablefmt='psql'), end='\n\n')


def get_table_df(dataset):
    return pd.read_csv(f"examples/datasets/ind_datasets/{dataset}.csv", header=[0])


def aind_str(lhs, rhs):
    def cc_str(cc):
        (table_name, df, indices) = cc
        columns = [df.columns[idx] for idx in indices]
        return ", ".join(f"{table_name}.{col}" for col in columns)

    return f"[{cc_str(lhs)}] -> [{cc_str(rhs)}]"


def aind_verify(lhs, rhs):
    (lhs_table_name, lhs_table, lhs_indices) = lhs
    (rhs_table_name, rhs_table, rhs_indices) = rhs

    print_table(lhs_table, f"Dataset '{lhs_table_name}':")
    print_table(rhs_table, f"Dataset '{rhs_table_name}':")

    ind_str = aind_str((lhs_table_name, lhs_table, lhs_indices),
                       (rhs_table_name, rhs_table, rhs_indices))

    print(f"Checking the IND {ind_str}")

    algo = desbordante.aind_verification.algorithms.Default()
    algo.load_data(tables=[lhs_table, rhs_table])
    algo.execute(lhs_indices=lhs_indices, rhs_indices=rhs_indices)

    return algo


def print_clusters(verifier, lhs):
    (table_name, table, indices) = lhs

    print(f"Number of clusters violating IND: {verifier.get_violating_clusters_count()}")
    for i, cluster in enumerate(verifier.get_violating_clusters(), start=1):
        print(f"{BLUE_CODE} #{i} cluster: {DEFAULT_COLOR_CODE}")
        for el in cluster:
            values = " ".join([f"{table[table.columns[idx]][el]}" for idx in indices])
            print(f"{el}: {values}")
    print(RESET_CODE)


def exact_scenario():
    prints("Let's start with the exact IND verification scenario.")
    prints("""The datasets under consideration for this scenario are 'orders'
and 'products'.""")
    print()
    prints("Let's start by verifying exact IND holding between those tables.")
    print()

    algo = aind_verify(('orders', get_table_df('orders'), [2]),
                       ('products', get_table_df('products'), [1]))

    print_results_for_ind(algo)
    print()
    prints("""The IND holds because there are no inconsistencies
between the two tables. The `products.name` column acts as a primary key,
and all values in the `orders.product` column match entries in `products.name`
without any typos or missing data.""")


def approximate_scenario():
    print("Now, let's consider the approximate IND verification scenario (AIND).")
    print()
    prints("""Unlike exact INDs, approximate INDs allow for a certain level of error.
This error indicates how accurately the dependency holds between the datasets.
In this scenario, we will use the 'orders' and 'customers' datasets.""")
    print()

    verify = lambda lhs_df, rhs_df: aind_verify(('orders', lhs_df, [1]),
                                                ('customers', rhs_df, [0]))

    lhs_df = get_table_df('orders')
    rhs_df = get_table_df('customers')

    algo = verify(lhs_df, rhs_df)

    print_results_for_ind(algo)
    print()
    prints(f"""We see that this AIND has an error of {algo.get_error():.2f}.
Let's examine the violating clusters in more detail to understand the errors.""")
    print()

    print_clusters(algo, ('orders', lhs_df, [1]))
    prints("Based on our analysis, this AIND does not hold due to the following reasons:")
    prints("""1. The `orders.customer_id` value '201' does not match any entry in the `customers.id` column.
This suggests a possible typo where '201' might have been entered instead of '101',
indicating that the customer who bought the 'Mouse' should be Alice.""")
    prints("""2. The `orders.customer_id` value '108' also violates the AIND.
This appears to be a case where the 'customers' table might be incomplete,
and some customer entries are missing.""")
    print()
    prints("""In such cases, resolving typos and ensuring data completeness in the reference table ('customers')
can help improve the accuracy of this dependency.""")

    print()
    print("Let's fix the issues.")
    print()
    prints("Step 1: Fix data issue in the 'orders' dataset.")
    prints("Update the value in the `orders.customer_id` column where it is '201' to '101'.")
    print()

    lhs_df.loc[lhs_df['customer_id'] == 201, 'customer_id'] = 101
    algo = verify(lhs_df, rhs_df)

    print_results_for_ind(algo)
    print()
    prints("""We have successfully fixed the typo in the 'orders' dataset. Now,
let's address the missing customer entry.""")

    print()
    prints("Step 2: Add the missing customer to the 'customers' dataset.")
    prints("Adding a new customer with id '108', name 'Frank', and country 'Italy'.")
    print()

    new_entry = {'id': 108, 'name': 'Frank', 'country': 'Italy'}
    rhs_df = pd.concat([rhs_df, pd.DataFrame([new_entry])], ignore_index=True)
    algo = verify(lhs_df, rhs_df)

    print_results_for_ind(algo)
    print()
    prints("The missing customer has been successfully added to the 'customers' dataset.")

    print()
    prints("All issues in the 'orders' and 'customers' datasets have been resolved.")


prints("""This example demonstrates two scenarios: verifying exact INDs and approximate
INDs (AINDs).""")


print()
exact_scenario()

print()
print("-" * 80)
print()

approximate_scenario()
