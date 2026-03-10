from os import path
import desbordante as db
import pandas as pd

print("""This is a basic example explaining how to use Approximate Denial Constraint (ADC) verification for checking hypotheses on data.
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

TABLE_1 = "datasets/taxes_3.csv"

print_table(TABLE_1)

# Creating a verifier and loading data
verifier = db.adc_verification.algorithms.Default()
verifier.load_data(table=(TABLE_1, ',', True))

# Algorithm execution, default measure is g1
verifier.execute(denial_constraint=DC, error=0.98)

# Obtaining the result
result: bool = verifier.adc_holds()

error: float = verifier.get_error()

print(f"DC {DC} holds: {result}\n")
print(f"Error: {error}")
