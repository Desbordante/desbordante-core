import desbordante as db
import pandas as pd

print("""This is a basic example explaining how to use Denial Constraint (DC) verification for checking hypotheses on data.
A more advanced example of using Denial Constraints is located in examples/expert/data_cleaning_dc.py.

DC verification is performed by the Rapidash algorithm:
Zifan Liu, Shaleen Deep, Anna Fariha, Fotis Psallidas, Ashish Tiwari, and Avrilia
Floratou. 2023. Rapidash: Efficient Constraint Discovery via Rapid Verification.
URL: https://arxiv.org/abs/2309.12436
""")

def print_table(table: str) -> None:
    data = pd.read_csv(table, header=None)
    data.index = data.index + 1
    headers = data.iloc[0]
    data = data[1:]
    data.columns = headers
    print(data, end='\n\n')

# The given denial constraint tells us that if two people live in the
# same state, the one earning a lower salary must have a lower tax rate
DC = "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)"

TABLE_1 = "examples/datasets/taxes_1.csv"
TABLE_2 = "examples/datasets/taxes_2.csv"

# If there is no need to collect violations, turn this
# option off for increased verification speed
DO_COLLECT_VIOLATIONS = True

print("""DC φ is a conjunction of predicates of the following form:
∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m)

DCs involve comparisons between pairs of rows within a dataset.
A typical DC example, derived from a Functional Dependency such as A -> B,
is expressed as: "∀s, t ∈ R, s ≠ t, ¬(t.A == s.A ∧ t.B ≠ s.B)".

This denotes that for any pair of rows in the relation, it should not be the case
that while the values in column A are equal, the values in column B are unequal.\n
Consider the following dataset:
""")

print_table(TABLE_1)

print("""And the following Denial Constraint:
!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate).\n
We use "and" instead of "∧" and "¬" instead of "!" for easier representation.

The constraint tells us that for all people in the same state a person with a higher salary has a higher tax rate.
Then we run the algorithm in order to see if the constraint holds.
""")

# Creating a verifier and loading data
verifier = db.dc_verification.algorithms.Default()
verifier.load_data(table=(TABLE_1, ',', True))

# Algorithm execution
verifier.execute(denial_constraint=DC, do_collect_violations=DO_COLLECT_VIOLATIONS)

# Obtaining the result
result: bool = verifier.dc_holds()

print(f"DC {DC} holds: {result}\n")

print("""Indeed, the given constraint holds. Now we will modify the initial dataset to make things
a little more interesting. Consider the previous table but with an additional record for Texas (#11):
""")

print_table(TABLE_2)

verifier = db.dc_verification.algorithms.Default()
verifier.load_data(table=(TABLE_2, ',', True))
verifier.execute(denial_constraint=DC, do_collect_violations=DO_COLLECT_VIOLATIONS)

result: bool = verifier.dc_holds()

print(f"DC {DC} holds: {result}\n")

# Get violations from the algorithm as a list of pairs
violations = verifier.get_violations()
viol_str = ", ".join(map(str, violations))

print(f"""Now we can see that the same DC we examined on the previous dataset doesn't hold on the new one.
The issue is that for the last record (Texas, 5000, 0.05), there are people in Texas with a lower salary
but a higher tax rate.

Such pairs of records that contradict a DC are called violations. We can retrieve these 
violations from the algorithm object. In this case, the following pairs are the violations:
{viol_str}, where each number is an index of a record in the table.
""")
