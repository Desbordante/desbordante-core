import csv
import textwrap

import desbordante
import pandas as pd

OUTPUT_WIDTH = 100

def wrap_text(text: str, width: int = OUTPUT_WIDTH) -> str:
    lines = text.split('\n')
    wrapped_lines = []
    for line in lines:
        if len(line) <= width:
            wrapped_lines.append(line)
        else:
            wrapped_lines.extend(textwrap.wrap(line, width=width, break_long_words=False, replace_whitespace=False))
    return '\n'.join(wrapped_lines)

def print_wrapped(text: str, width: int = OUTPUT_WIDTH) -> None:
    print(wrap_text(text, width))

def print_cdd_wrapped(cdd_obj, width: int = OUTPUT_WIDTH) -> None:
    print_wrapped(str(cdd_obj), width)

COLOR_CODES = {
    "green": "\033[1;32m",
    "red": "\u001b[1;31m",
    "yellow": "\033[1;33m",
    "cyan": "\033[1;36m",
    "nocolor": "\033[0m"
}

def print_holds(result: bool) -> None:
    print_wrapped("Let`s verify this dependency.")
    if result:
        print_wrapped(f'This {COLOR_CODES["green"]}CDD holds.{COLOR_CODES["nocolor"]}\n')
    else:
        print_wrapped(f'This {COLOR_CODES["red"]}CDD doesn`t hold.{COLOR_CODES["nocolor"]}\n')

def print_dd_highlights(dd_highlights, num_attrs, table):
    if not dd_highlights:
        print_wrapped(f"{COLOR_CODES['green']}No DD violations found.{COLOR_CODES['nocolor']} (The distance constraints are satisfied).")
    else:
        print_wrapped(f"{COLOR_CODES['red']}DD violations found: {len(dd_highlights)} pairs.{COLOR_CODES['nocolor']}")
        table_copy: list[list[str]]
        with open(table, newline='') as act_table:
            act = list(csv.reader(act_table))
            table_copy = act.copy()
        for highlight in dd_highlights:
            first_error_row_index = highlight.pair_rows[0] + 1
            second_error_row_index = highlight.pair_rows[1] + 1
            row1_parts = []
            for i in range(0, num_attrs):
                if i == highlight.attribute_index:
                    if i == num_attrs - 1:
                        row1_parts.append(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]}')
                    else:
                        row1_parts.append(f'{COLOR_CODES["red"]}{table_copy[first_error_row_index][i]}{COLOR_CODES["nocolor"]}')
                else:
                    row1_parts.append(table_copy[first_error_row_index][i])
            print_wrapped(f"{first_error_row_index}) " + " ".join(row1_parts))
            
            row2_parts = []
            for i in range(0, num_attrs):
                if i == highlight.attribute_index:
                    if i == num_attrs - 1:
                        row2_parts.append(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]}')
                    else:
                        row2_parts.append(f'{COLOR_CODES["red"]}{table_copy[second_error_row_index][i]}{COLOR_CODES["nocolor"]}')
                else:
                    row2_parts.append(table_copy[second_error_row_index][i])
            print_wrapped(f"{second_error_row_index}) " + " ".join(row2_parts))
            print_wrapped("")

def print_cdd_highlights(cdd_highlights, num_attrs, table):
    if not cdd_highlights:
        print_wrapped(f"{COLOR_CODES['green']}No RHS condition violations found.{COLOR_CODES['nocolor']}")
    else:
        print_wrapped(f"{COLOR_CODES['red']}RHS condition violations found: {len(cdd_highlights)} rows.{COLOR_CODES['nocolor']}\n")
        table_copy: list[list[str]]
        with open(table, newline='') as act_table:
            act = list(csv.reader(act_table))
            table_copy = act.copy()
        for h in cdd_highlights:
            row_index = h.row_index + 1
            row_parts = []
            for i in range(0, num_attrs):
                if i == h.condition_index:
                    row_parts.append(f'{COLOR_CODES["red"]}{table_copy[row_index][i]}{COLOR_CODES["nocolor"]}')
                else:
                    row_parts.append(table_copy[row_index][i])
            print_wrapped(f"{row_index}) " + " ".join(row_parts))
        print_wrapped("")

def print_intro(table_path):
    intro_text = '''Validating Conditional Differential Dependencies (CDDs)
=======================================================

This is an example of validating Conditional Differential Dependencies (CDDs).
In Desbordante, there are also examples of validating and mining
differential dependencies (DDs). Before continuing with the current
example, we recommend that you take a look at those examples (see
the links below).

CDDs were introduced by Selasi Kwashie, Jixue Liu, Jiuyong Li, and Feiyue Ye
in their 2015 article "Conditional Differential Dependencies (CDDs)",
published in Advances in Databases and Information Systems. ADBIS 2015.
Lecture Notes in Computer Science, vol 9282.

What is a CDD?
--------------
A CDD extends the concept of DD by adding conditions that restrict the
scope in which the dependency applies.

A CDD consists of:
1. A standard DD, expressed in the form LHS -> RHS, where LHS is the 
   left-hand side and RHS is the right-hand side. 
2. LHS conditions: these act as a filter for the entire table. Only rows
   satisfying these conditions are considered for the DD check.
3. RHS conditions: for a CDD to hold, tuple pairs satisfying the LHS DD
   must also satisfy these conditions, in addition to satisfying the RHS
   interval of the DD.

Note that a CDD without defined conditions is equivalent to the
corresponding DD.

Supported Condition Predicates
------------------------------
EQ (=)          - equality
NEQ (!=)        - inequality
LT (<)          - less than
LE (<=)         - less than or equal
GT (>)          - greater than
GE (>=)         - greater than or equal
IN_SET (∈)      - value in a set (e.g., category ∈ {{Smartphones, Laptops}})
IN_INTERVAL (∈) - value in an interval (e.g., price ∈ [100, 500])
ANY (_)         - wildcard, matches any value

Conditions can be specified on any table attributes, including those not 
participating in the DD. The LHS conditions are a set of predicates 
combined with AND. The RHS follows the same pattern.

Exceptions
----------
If a dependency does not hold, the verifier can also find outliers 
(exceptions), i.e. the data that violate it.

The notion of an exception in CDD differs from that in DD and is somewhat 
counterintuitive. It also differs from other conditional patterns (CFD, 
CIND) because of differences in how the pattern is defined.

DD: an exception is a pair of rows whose distance on the LHS attributes 
lies within the LHS interval, but whose distance on the RHS attributes 
falls outside the RHS interval.

CDD: there are two kinds of exceptions:
   
  1. The same kind of exceptions as in DD, but searched for only among 
     rows that satisfy the LHS condition. In other words, these are row 
     pairs that fit into the LHS interval and satisfy the LHS condition, 
     but do not fit into the RHS interval.

  2. Rows that fit into the LHS interval and satisfy the LHS condition, 
     but do not satisfy the RHS condition.

To explain the CDD concept and illustrate the validation of this primitive,
we will examine the stores_dd.csv dataset.
'''
    print_wrapped(intro_text)
    data = pd.read_csv(table_path)
    print_wrapped(data.to_string() + "\n")

def run_example_1(table_path):
    print_wrapped("\n" + "-"*100)
    print_wrapped("Example #1: Basic validation (CDD without conditions equals DD)")
    print_wrapped("-"*100)
    
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

    print_wrapped(f"""As previously noted, when conditions are not specified for a CDD, it is 
equivalent to the corresponding DD.

Consider the following CDD:

{COLOR_CODES['yellow']}{dd_str}, LHS conditions: ({lhs_cond_str}), RHS conditions: ({rhs_cond_str}){COLOR_CODES['nocolor']}
""")
    print_holds(algo.dd_holds())
    print_wrapped(f"""As explained in the DD verification example, this dependency defines constraints
on the distance between pairs of attribute values. In other words, this dependency 
demonstrates that for any identical product across all stores, the difference in 
stock_quantity does not exceed 20 units, and the price difference does not exceed 
60 units.""")

def run_example_2(table_path):
    print_wrapped("\n" + "-"*100)
    print_wrapped("Example #2: LHS conditions (restricting the scope of a dependency)")
    print_wrapped("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("price_per_unit", 0, 40)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    
    
    cond_lhs = [desbordante.cdd.Condition("category", 'Smartphones', desbordante.cdd.ConditionOp.EQ)]
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=[])
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)
    
    print_wrapped(f"""In practice, verifying a dependency to hold over the entire table may be too strong 
condition: it often holds only on some subsets of rows rather than on all of them. 
For example, it can be observed that, the price difference for Smartphones is less 
than 60 — a number we used in the example above.

Consider the following CDD:

{COLOR_CODES['yellow']}{cdd}{COLOR_CODES['nocolor']}
""")
    
    print_wrapped(f"""This dependency means that for identical smartphones, the price difference between any
two stores does not exceed 40 units.\n""")
    
    data = pd.read_csv(table_path)
    mask = data['category'] == 'Smartphones'

    lines = data.to_string().split('\n')
    header, rows = lines[0], lines[1:]
    print_wrapped(f"Rows satisfying the LHS condition (category = Smartphones):\n")
    print_wrapped(header)
    for line, is_match in zip(rows, mask):
        print_wrapped(f"{COLOR_CODES['green']}{line}{COLOR_CODES['nocolor']}" if is_match else line)
    
    print_wrapped("")
    print_holds(algo.dd_holds())
    print_wrapped("""This validates our hypothesis.
At the same time, without setting additional conditions, the CDD fails to hold.
""")
    
    cdd_without_conds = desbordante.cdd.CDD(dd=dd, lhs_condition=[], rhs_condition=[])
    algo.execute(cdd=cdd_without_conds)
    print_wrapped(f'{COLOR_CODES["yellow"]}{cdd_without_conds}{COLOR_CODES["nocolor"]}\n')
    print_holds(algo.dd_holds())
    print_wrapped(f"""Let's examine which rows prevent this dependency from holding.
    """)
    print_dd_highlights(algo.get_highlights(), 5, table_path)
    print_wrapped(f"""Note that, in this example, the exception set contains only pairs of rows; there 
are no single-row violations.
""")


def run_example_3(table_path):
    print_wrapped("-"*100)
    print_wrapped("Example #3: RHS conditions (an additional constraint on matching pairs)")
    print_wrapped("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("stock_quantity", 0, 1000)] 
    dd = desbordante.dd.DD(lhs, rhs)
    
    cond_rhs = [desbordante.cdd.Condition("store_name", "Walmart TX", desbordante.cdd.ConditionOp.EQ)]
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=[], rhs_condition=cond_rhs)
    
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)
    print_wrapped("""Unlike LHS conditions, which prune the tuples under consideration, RHS conditions
act as an additional constraint that matching pairs must satisfy. In order for a 
CDD to hold, every pair satisfying the LHS DD must additionally satisfy the RHS 
conditions of the CDD.

Consider the CDD:
""")
    print_wrapped(f"{COLOR_CODES['yellow']}{cdd}{COLOR_CODES['nocolor']}\n")
    print_holds(algo.dd_holds())
    print_wrapped("""While the DD defined by this CDD holds, some of the rows that satisfy the distance 
constraints of the RHS DD violate the RHS conditions of the CDD.
""")
    cdd_highlights = algo.get_cdd_highlights()
    print_cdd_highlights(cdd_highlights, 5, table_path)

def run_example_4(table_path):
    print_wrapped("-"*100)
    print_wrapped("Example #4: Detecting and fixing logical errors in a data segment")
    print_wrapped("-"*100)
    
    lhs = [desbordante.dd_verification.DF("origin_hub", 0, 0), desbordante.dd_verification.DF("dest_hub", 0, 0), desbordante.dd_verification.DF("weight_kg", 0, 1.5)]
    rhs = [desbordante.dd_verification.DF("transit_hours", 0, 4)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    cond_lhs = [desbordante.cdd.Condition("delivery_type", "Express", desbordante.cdd.ConditionOp.EQ)]
    cdd = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=[])
    
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd)
    
    print_wrapped("""CDDs allow to detect logical errors within specific segments of data.

Consider the logistics_cdd.csv table:
""")
    data = pd.read_csv(table_path)
    print_wrapped(data.to_string() + "\n")

    print_wrapped("""Under company business policies, express shipments with identical origin and destination hubs
and comparable weights (within 1.5 kg) are expected to have a transit_hours difference of no 
more than 4 hours.

We evaluate this expected behavior with the following CDD:

""")
    print_wrapped(f"{COLOR_CODES['yellow']}{cdd}{COLOR_CODES['nocolor']}\n")
    print_holds(algo.dd_holds())
    
    dd_highlights = algo.get_highlights()
    print_dd_highlights(dd_highlights, 6, table_path)
    print_wrapped("""It can be seen that 103 package violates express shipment business policies. Consequently, it 
is logical to assume that an inaccuracy occurred and the package was intended to have the 
Standard delivery type. We now correct this error.
""")
    data = pd.read_csv('examples/datasets/logistics_cdd_1.csv')
    print_wrapped(data.to_string() + "\n")
    
    algo_corr = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo_corr.load_data(table=('examples/datasets/logistics_cdd_1.csv', ',', True))
    algo_corr.execute(cdd=cdd)
    
    print_holds(algo_corr.dd_holds())

def run_example_5(table_path):
    print_wrapped("\n" + "-"*100)
    print_wrapped("Example #5: Combining LHS and RHS conditions (business constraint violation)")
    print_wrapped("-"*100)
    
    lhs = [desbordante.dd_verification.DF("product_name", 0, 0)]
    rhs = [desbordante.dd_verification.DF("price_per_unit", 0, 40)]
    dd = desbordante.dd.DD(lhs, rhs)
    
    cond_lhs = [desbordante.cdd.Condition("category", "Smartphones", desbordante.cdd.ConditionOp.EQ)]
    
    cdd1 = desbordante.cdd.CDD(dd=dd, lhs_condition=cond_lhs, rhs_condition=[])
    algo = desbordante.cdd_verification.algorithms.CDDVerifier()
    algo.load_data(table=(table_path, ',', True))
    algo.execute(cdd=cdd1)
    
    print_wrapped(f"""LHS conditions answer the question "which rows does the rule apply to?", while RHS conditions
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
    print_wrapped(f"""Prices of identical smartphones are consistent across stores, so the DD part does not find
anything suspicious. Now add the MAP policy as an RHS condition:

{COLOR_CODES['yellow']}{cdd2}{COLOR_CODES['nocolor']}
""")
 
    
    algo.execute(cdd=cdd2)
    
    print_holds(algo.dd_holds())
    
    print_wrapped(f"""While the DD defined by this CDD holds, some of the rows that satisfy the distance
constraints of the RHS DD violate the RHS conditions of the CDD.
""")
    cdd_highlights = algo.get_cdd_highlights()
    print_cdd_highlights(cdd_highlights, 5, table_path)
    
    print_wrapped(f"""Both violations concern Samsung Galaxy S23: the prices are close to the other stores, but below
the MAP threshold. We correct the prices.
""")
    
    corrected_path = 'examples/datasets/stores_dd_1.csv'
    data = pd.read_csv(corrected_path)
    print_wrapped(data.to_string() + "\n")
    
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

print_wrapped("""
Related examples (similar primitives):
examples/basic/verifying_dd.py   - Differential Dependencies verification
examples/basic/verifying_cfd.py  - Conditional Functional Dependencies verification
examples/basic/mining_dd.py      - Differential Dependencies mining
examples/basic/mining_cfd.py     - Conditional Functional Dependencies mining
""")
