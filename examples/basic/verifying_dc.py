import desbordante as db
import pandas as pd

# TODO: Add console interaction with user

# The given denial constraint tells us that if two persons live in the
# same state, the one earning a lower salary has a lower tax rate
DC = "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)"

TABLE_1 = "examples/datasets/taxes.csv"
TABLE_2 = "examples/datasets/taxes_2.csv"

print("""This is an example of how to use Denial Constraint (DC) verification for checking hypotheses on data.
DC verification is perfomed by the Rapidash algorithm: https://arxiv.org/abs/2309.12436
DC φ is a conjunction of predicates of the following form:
∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m)

DCs involve comparisons between pairs of rows within a dataset.
A typical DC example, derived from a Functional Dependency such as A -> B,
is expressed as: "∀s, t ∈ R, s ≠ t, ¬(t.A == s.A and t.B ≠ s.B).
This denotes that for any pair of rows in the relation, it should not be the case
that while the values in column A are equal, the values in column B are unequal.
""")

print("Consider the following dataset:\n")
data = pd.read_csv(TABLE_1, header=[0])
print(data, end="\n\n")
print("""And the following Denial Constraint: !(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate).
It tells us that for all people in the same state the person with a higher salary has a higher tax rate.
Then we run the algorithm in order to see if the constraint holds.
""")

# Creating a verifier and loading data
verifier = db.dc_verification.algorithms.Default()
verifier.load_data(table=(TABLE_1, ',', True))

# Algorithm execution
verifier.execute(denial_constraint=DC)

# Obtaining the result
result: bool = verifier.dc_holds()

print(f"DC {DC} holds: {result}\n")

print("""Now we will modify the initial dataset to make things a little bit more interesting.
Consider the previous table but with an additional record for Texas:
""")

data = pd.read_csv(TABLE_2, header=[0])
print(data, end="\n\n")

verifier = db.dc_verification.algorithms.Default()
verifier.load_data(table=(TABLE_2, ',', True))
verifier.execute(denial_constraint=DC)

result: bool = verifier.dc_holds()

print(f"DC {DC} holds: {result}\n")

print("""Now we can see that the same DC we examined on the previous dataset doesn't hold on the new one.
The thing is that for the last record (Texas, 5000, 0.05) there are people in Texas with a lower salary
but higher tax rate. Pairs of records like this that contradict a DC are called violations.
In this case the following pairs are violations:
(6, 9), (7, 9), (8, 9), where each number is an index of a record.
""")
