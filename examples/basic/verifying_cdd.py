import csv

import desbordante
import pandas as pd

COLOR_CODES = {
    "green": "\033[1;32m",
    "red": "\u001b[1;31m",
    "yellow": "\033[1;33m",
    "cyan": "\033[1;36m",
    "nocolor": "\033[0m"
}

def print_holds(result: bool) -> None:
    print("Let`s verify this.")
    if result:
        print(f'This {COLOR_CODES["green"]}CDD holds.{COLOR_CODES["nocolor"]}\n')
    else:
        print(f'This {COLOR_CODES["red"]}CDD doesn`t hold.{COLOR_CODES["nocolor"]}\n')

def print_dd_highlights(dd_highlights, num_attrs, table):
    if not dd_highlights:
        print(f"- {COLOR_CODES['green']}No DD violations found.{COLOR_CODES['nocolor']} (The distance constraints are satisfied).")
    else:
        print(f"- {COLOR_CODES['red']}DD violations found: {len(dd_highlights)} pairs.{COLOR_CODES['nocolor']}")
        table_copy: list[list[str]]
        with open(table, newline='') as act_table:
            act = list(csv.reader(act_table))
            table_copy = act.copy()
        for highlight in dd_highlights:
            first_error_row_index = highlight.pair_rows[0] + 1
            second_error_row_index = highlight.pair_rows[1] + 1
            print(f"{first_error_row_index}) ", end="")
            for i in range(0, num_attrs):
                if i == highlight.attribute_index:
                    if i == num_attrs - 1:
                        print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                    else:
                        print(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                else:
                    if i == num_attrs - 1:
                        print(table_copy[first_error_row_index][i], end="")
                    else:
                        print(table_copy[first_error_row_index][i] + " ", end="")
            print()
            print(f"{second_error_row_index}) ", end="")
            for i in range(0, num_attrs):
                if i == highlight.attribute_index:
                    if i == num_attrs - 1:
                        print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                    else:
                        print(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                else:
                    if i == num_attrs - 1:
                        print(table_copy[second_error_row_index][i], end="")
                    else:
                        print(table_copy[second_error_row_index][i] + " ", end="")
            print("\n")

def print_cdd_highlights(cdd_highlights, num_attrs, table):
    if not cdd_highlights:
        print(f"{COLOR_CODES['green']}No RHS condition violations found.{COLOR_CODES['nocolor']}")
    else:
        print(f"{COLOR_CODES['red']}RHS condition violations found: {len(cdd_highlights)} rows.{COLOR_CODES['nocolor']}\n")
        table_copy: list[list[str]]
        with open(table, newline='') as act_table:
            act = list(csv.reader(act_table))
            table_copy = act.copy()
        for h in cdd_highlights:
            row_index = h.row_index + 1
            print(f"{row_index}) ", end="")
            for i in range(0, num_attrs):
                if i == h.condition_index:
                    if i == num_attrs - 1:
                        print(f'{COLOR_CODES["red"]}{table_copy[row_index][i]}{COLOR_CODES["nocolor"]}', end="")
                    else:
                        print(f'{COLOR_CODES["red"]}{table_copy[row_index][i]}{COLOR_CODES["nocolor"]} ', end="")
                else:
                    if i == num_attrs - 1:
                        print(table_copy[row_index][i], end="")
                    else:
                        print(table_copy[row_index][i] + " ", end="")
            print()
        print("\n")

def print_intro(table_path):
    print(
        f'''This is an example of validating Conditional Differential Dependencies (CDDs).
In Desbordante, there are also examples of validating and mining 
Differential Dependencies (DD). Before viewing these examples, we 
recommend that you take a look at the existing ones.

CDDs were introduced by Selasi Kwashie, Jixue Liu, Jiuyong Li, Feiyue in 
their 2015 article, "Conditional Differential Dependencies (CDDs)", 
published in Advances in Databases and Information Systems. ADBIS 2015. 
Lecture Notes in Computer Science, vol 9282.

A CDD extends the concept of DD by adding conditions that restrict the 
scope in which the dependency applies.

A CDD consists of:
1. A standard DD (LHS -> RHS).
2. LHS Conditions: These act as a filter for the entire table. Only rows 
satisfying these conditions are considered for the DD check.
3. RHS Conditions: For a CDD to hold, tuple pairs satisfying the LHS DD 
must also satisfy these conditions, in addition to satisfying the RHS 
conditions of the DD.

It is important that a CDD without defined conditions becomes equivalent 
to the corresponding DD.

Supported condition predicates:
EQ (=)       - equality
NEQ (!=)     - inequality  
LT (<)       - less than
LE (<=)      - less than or equal
GT (>)       - greater than
GE (>=)      - greater than or equal
IN_SET (∈)   - value in a set (e.g., category ∈ {{Smartphones, Laptops}})
IN_INTERVAL (∈) - value in an interval (e.g., price ∈ [100, 500])
ANY (_)      - wildcard, matches any value

Conditions can be specified on any table attributes, including those not 
participating in the DD.

To explain the CDD concept and illustrate the validation of this primitive,
we will examine the stores_dd.csv dataset.
''')
    data = pd.read_csv(table_path)
    print(f"{data}\n")

def run_example_1(table_path):
    print("\n" + "-"*100)
    print("Example #1")
    print("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("stock_quantity", 0, 20), desbordante.dd_verification.DF("price_per_unit", 0, 60)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=[], rhs_condition=[])
    
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)

    lhs_str = " ; ".join(f"{df.attribute_name} [{df.lower_bound}, {df.upper_bound}]" for df in cdd.dd.left)
    rhs_str = " ; ".join(f"{df.attribute_name} [{df.lower_bound}, {df.upper_bound}]" for df in cdd.dd.right)
    dd_str = f"{lhs_str} -> {rhs_str}"

    lhs_cond_str = ", ".join(f"{c.attribute} {c.op}" for c in cdd.lhs_condition) if cdd.lhs_condition else ""
    rhs_cond_str = ", ".join(f"{c.attribute} {c.op}" for c in cdd.rhs_condition) if cdd.rhs_condition else ""

    print(f"""As previously noted, when conditions are not specified for a CDD, it is 
equivalent to the corresponding DD.

Consider the following CDD:

{COLOR_CODES['yellow']}{dd_str}, LHS conditions: ({lhs_cond_str}), RHS conditions: ({rhs_cond_str}){COLOR_CODES['nocolor']}
""")
    print_holds(algo.dd_holds())
    print(f"""As explained in the DD verification example, this dependency defines constraints
on the distance between pairs of attribute values. In other words, this dependency 
demonstrates that for any identical product across all stores, the variance in 
stock_quantity does not exceed 20 units, and the price difference does not exceed 
60 units.""")

def run_example_2(table_path):
    print("\n" + "-"*100)
    print("Example #2")
    print("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("price_per_unit", 0, 40)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    
    
    cond_lhs = [desbordante.cdd.Condition("category", 'Smartphones', desbordante.cdd.ConditionOp.EQ)]
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=[])
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)
    
    print(f"""In practice, verifying a dependency to hold over the entire table may be too strong 
condition: it often holds only on some subsets of rows rather than on all of them. 
For example, it can be observed that, the price difference for Smartphones is less 
than 60.

Consider the following CDD:

{COLOR_CODES['yellow']}{cdd}{COLOR_CODES['nocolor']}
""")
    
    print(f"Interpretation: For identical smartphones across different stores, the price difference does not exceed 40 units.\n")
    
    data = pd.read_csv(table_path)
    smartphones = data[data['category'] == 'Smartphones']
    print(f"Rows satisfying the LHS condition (category = Smartphones):")
    print(f"{smartphones.to_string(index=False)}\n")
    
    print_holds(algo.dd_holds())
    print("""This validates our hypothesis.
At the same time, without setting additional conditions, the CDD fails to hold.
""")
    
    print(f"The verifier supports finding exceptions for dependencies that don't hold."
          f"\nFor DD, exceptions are row pairs where the distance between considered \nattributes "
          f"satisfies LHS but violates RHS DD. "
          f"For CDDs, exceptions are rows\nthat belong to pairs satisfying LHS DD, "
          f"but for which the RHS Condition is not satisfied.\n")
    
    cdd_without_conds = desbordante.cdd.CDD(dd=dd, lhs_condition=[], rhs_condition=[])
    algo.execute(cdd=cdd_without_conds)
    print(f'{COLOR_CODES["yellow"]}{cdd_without_conds}{COLOR_CODES["nocolor"]}\n')
    print_holds(algo.dd_holds())
    print_dd_highlights(algo.get_highlights(), 5, table_path)


def run_example_3(table_path):
    print("-"*100)
    print("Example #3")
    print("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("stock_quantity", 0, 1000)] # Very loose DD, likely holds
    dd = desbordante.dd.DD(lhs, rhs)
    
    cond_rhs = [desbordante.cdd.Condition("store_name", "Walmart TX", desbordante.cdd.ConditionOp.EQ)]
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=[], rhs_condition=cond_rhs)
    
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)
    print("""Unlike LHS conditions, which prune the tuples under consideration, RHS conditions
act as an additional constraint that matching pairs must satisfy. In order for a 
CDD to hold, every pair satisfying the LHS DD must additionally satisfy the RHS 
conditions of the CDD.

Consider the CDD:
""")
    print(f"{COLOR_CODES['yellow']}{cdd}{COLOR_CODES['nocolor']}\n")
    print_holds(algo.dd_holds())
    print("""While the DD defined by this CDD holds, some of the rows that satisfy the distance 
constraints of the RHS DD violate the RHS conditions of the CDD.
""")
    cdd_highlights = algo.get_cdd_highlights()
    print_cdd_highlights(cdd_highlights, 5, table_path)

def run_example_4(table_path):
    print("-"*100)
    print("Example #4")
    print("-"*100)
    
    lhs = [desbordante.dd_verification.DF("origin_hub", 0, 0), desbordante.dd_verification.DF("dest_hub", 0, 0), desbordante.dd_verification.DF("weight_kg", 0, 1.5)]
    rhs = [desbordante.dd_verification.DF("transit_hours", 0, 4)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    cond_lhs = [desbordante.cdd.Condition("delivery_type", "Express", desbordante.cdd.ConditionOp.EQ)]
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=[])
    
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)
    
    print("""CDDs allow to detect logical errors within specific segments of data.

Consider the logistics_cdd.csv table:
""")
    data = pd.read_csv(table_path)
    print(f"{data}\n")

    print("""Under company business policies, express shipments with identical origin and destination hubs
and comparable weights (within 1.5 kg) are expected to have a transit_hours difference of no 
more than 4 hours.

We evaluate this expected behavior with the following CDD:

""")
    print(f"{COLOR_CODES['yellow']}{cdd}{COLOR_CODES['nocolor']}\n")
    print_holds(algo.dd_holds())
    
    dd_highlights = algo.get_highlights()
    print_dd_highlights(dd_highlights, 6, table_path)
    print("""It can be seen that 103 package violates express shipment business policies. Consequently, it 
is logical to assume that an inaccuracy occurred and the package was intended to have the 
Standard delivery type. We now correct this error.
""")
    data = pd.read_csv('examples/datasets/logistics_cdd_1.csv')
    print(f"{data}\n")
    

    algo_corr = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo_corr.load_data(table=('examples/datasets/logistics_cdd_1.csv', ',', True))
    algo_corr.execute(cdd=cdd)
    
    print_holds(algo_corr.dd_holds())

def run_example_5(table_path):
    print("\n" + "-"*100)
    print("Example #5")
    print("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("price_per_unit", 0, 40)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    cond_lhs = [desbordante.cdd.Condition("category", "Smartphones", desbordante.cdd.ConditionOp.EQ)]
    
    cdd1 = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=[])
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd1)
    
    print(f"""LHS conditions answer the question "which rows does the rule apply to?", while RHS conditions
answer "what must additionally hold for the rows that match the rule?". This is useful when
values are consistent with each other, but violate an absolute business constraint:
a plain DD cannot detect this.

Let's return to the stores_dd.csv table.

Suppose the manufacturer enforces a minimum advertised price (MAP) policy: smartphones must
not be sold below 900 in any store, and prices of the same smartphone across stores must stay
close to each other (difference <= 40).

First, let's check only the consistency of prices inside the Smartphones segment:

{COLOR_CODES['yellow']}{cdd1}{COLOR_CODES['nocolor']}
""")
    print_holds(algo.dd_holds())
    
    cond_rhs = [desbordante.cdd.Condition("price_per_unit", 900, desbordante.cdd.ConditionOp.GE)]
    cdd2 = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=cond_rhs)
    print(f"""Prices of identical smartphones are consistent across stores, so the DD part does not find
anything suspicious. Now add the MAP policy as an RHS condition:

{COLOR_CODES['yellow']}{cdd2}{COLOR_CODES['nocolor']}
""")

    
    algo.execute(cdd=cdd2)
    
    print_holds(algo.dd_holds())
    
    print(f"""While the DD defined by this CDD holds, some of the rows that satisfy the distance
constraints of the RHS DD violate the RHS conditions of the CDD.
""")
    cdd_highlights = algo.get_cdd_highlights()
    print_cdd_highlights(cdd_highlights, 5, table_path)
    
    print(f"""Both violations concern Samsung Galaxy S23: the prices are close to the other stores, but below
the MAP threshold. We correct the prices.
""")
    
    corrected_path = 'examples/datasets/stores_dd_1.csv'
    data = pd.read_csv(corrected_path)
    print(f"{data}\n")
    
    algo_corr = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo_corr.load_data(table=(corrected_path, ',', True))
    algo_corr.execute(cdd=cdd2)
    print_holds(algo_corr.dd_holds())

if __name__ == "__main__":
    TABLE_STORES = 'examples/datasets/stores_dd.csv'
    TABLE_LOGISTICS = 'examples/datasets/logistics_cdd.csv'
    print_intro(TABLE_STORES)
    run_example_1(TABLE_STORES)
    run_example_2(TABLE_STORES)
    run_example_3(TABLE_STORES)
    run_example_4(TABLE_LOGISTICS)
    run_example_5(TABLE_STORES)

print("""
Related examples (similar primitives):
examples/basic/verifying_dd.py   - Differential Dependencies verification
examples/basic/verifying_cfd.py  - Conditional Functional Dependencies verification
examples/basic/mining_dd.py      - Differential Dependencies mining
examples/basic/mining_cfd.py     - Conditional Functional Dependencies mining
""")
