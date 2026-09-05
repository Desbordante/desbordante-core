import csv
import desbordante as db


GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[1m\033[36m"
ENDC = "\033[0m"

DEFAULT_DC = f"!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)"
DEFAULT_TABLE = "examples/datasets/taxes_2.csv"
DEFAULT_EPSILON = 0.035

MEASURES = ["g1", "g1_norm", "g2"]


def read_csv_table(filename: str) -> list[list[str]]:
    with open(filename, newline="") as csv_file:
        return list(csv.reader(csv_file))


def print_table(filename: str, title: str = "") -> None:
    rows = read_csv_table(filename)
    if title:
        print(title)

    table = [["row", *rows[0]]] + [[str(i), *row] for i, row in enumerate(rows[1:], 2)]
    widths = [max(len(row[column]) for row in table) for column in range(len(table[0]))]

    for row_index, row in enumerate(table):
        print("  ".join(cell.ljust(widths[column]) for column, cell in enumerate(row)))
        if row_index == 0:
            print("  ".join("-" * width for width in widths))
    print()


def verify_adc(
    table: str, dc: str, epsilon: float, measure: str
) -> tuple[bool, float, list[tuple[int, int]]]:
    verifier = db.adc_verification.algorithms.Default()
    verifier.load_data(table=(table, ",", True))
    verifier.execute(denial_constraint=dc, error=epsilon, adc_error_measure=measure)
    return verifier.adc_holds(), verifier.get_error(), verifier.get_violations()


def print_verification_results(table: str, dc: str, epsilon: float) -> None:
    print(f"{YELLOW}Verifying one ADC with different measures{ENDC}")
    print(f"ADC: {CYAN}{dc}{ENDC}")
    print(f"epsilon: {epsilon}\n")

    results = []
    violations = []
    for measure in MEASURES:
        holds, error, v = verify_adc(table, dc, epsilon, measure)
        violations.append(v)
        results.append((measure, holds, error))

    # The violating tuple pairs come from the denial constraint itself, not from the
    # error measure. Only the final error value changes between measures.
    for i in range(len(violations) - 1):
        assert violations[i] == violations[i + 1]

    print("measure    error     result")
    print("--------   -------   ----------------")
    for measure, holds, error in results:
        color = GREEN if holds else RED
        verdict = "holds" if holds else "does not hold"
        print(f"{measure:<8}  {error:<7.3f}   {color} {verdict}{ENDC}")
    print()

    print(f"violating ordered pairs: {', '.join(map(str, violations[0])) if violations[0] else 'none'}")
    print()


def main() -> None:
    print(f"""{YELLOW}Approximate Denial Constraint verification{ENDC}
This example verifies an Approximate Denial Constraint (ADC) using the Rapidash
verification algorithm. Rapidash provides the base for finding DC violations and
the ADC verifier then computes error metrics over those violations. The exact DC
verifier is demonstrated in \"examples/basic/verifying_dc.py\".

The algorithm is described in:
    Z. Liu et al. Rapidash: Efficient Constraint Discovery via Rapid Verification.
    2023. https://arxiv.org/abs/2309.12436

The ADC error measures follow:
    X. Xiao et al. Fast approximate denial constraint discovery. 2022.
    https://arxiv.org/abs/2312.06296

An ADC allows some violations of a DC. The allowed amount is controlled by
epsilon and by the selected error measure.

We will examine the following denial constraint:
DC: {CYAN}{DEFAULT_DC}{ENDC}

The constraint tells us that for all people in the same state a person with a higher
salary has a higher tax rate.
""")

    print_table(DEFAULT_TABLE, f"{YELLOW}Dataset: {DEFAULT_TABLE}{ENDC}")
    print("""The exact denial constraint does not hold on this dataset. For example,
row 8 (Texas, salary 1000, tax rate 0.15) and row 11 (Texas, salary 5000, tax
rate 0.05) form a violation: the higher salary has the lower tax rate.

Rapidash computes the violation set for the DC, and the ADC layer uses that set
to calculate the selected error metric. Thus, ADC verification keeps the same
violations as exact DC verification, but accepts the constraint when the
selected error stays below epsilon. The algorithm exposes these parameters:

    table=(filename, separator, has_header)
        The CSV file, its field separator, and whether the first row is a header.

    denial_constraint=dc
        The DC expression to verify. It must use the column names from the table.

    error=epsilon
        The maximum allowed error. It is a number from 0 to 1. 0 requires an exact
        DC, while a larger value allows more violations.

    adc_error_measure=measure
        One of "g1", "g1_norm", or "g2". It selects how violations are counted.
""")

    print(f"""{YELLOW}What error measures are available?{ENDC}
Desbordante computes an error based on the set of violations. An ADC holds when
that error is not greater than user-defined epsilon.

The returned error values are in [0, 1]. Zero means an exact DC. A value closer
to one means that very little of the DC remains true. Let n be the number of
rows, V be the set of violating ordered tuple pairs, and freq(r) be the
frequency of a distinct row value in the table.

  {CYAN}g1{ENDC}:
    error = |V| / n^2
        Counts violating ordered tuple pairs.

  {CYAN}g1_norm{ENDC}:
    error = |V| / (n^2 - sum(freq(r)^2)), r in R
        Uses the same violations as g1, but excludes pairs of equal duplicate rows
        from the denominator. Its error can therefore be larger than g1.

  {CYAN}g2{ENDC}:
    error = sum(freq(r)) / n^2, for distinct row values in violating tuples
        Counts rows participating in at least one violation, rather than counting
        violating pairs themselves.
""")

    print_verification_results(DEFAULT_TABLE, DEFAULT_DC, DEFAULT_EPSILON)

    print(f"""{YELLOW}Conclusion{ENDC}
The same ADC can hold for one measure and fail for another. With epsilon =
{DEFAULT_EPSILON}, this dataset is accepted by g1 and g1_norm, but rejected by g2
because the violations touch too many rows according to that metric.

Changing important inputs changes the result:
    - Increasing epsilon can turn a failing ADC into a passing one, decreasing it
        makes verification stricter.
    - Changing the measure changes the error even though the violation pairs stay
        the same. Duplicate rows matter especially for g1_norm, while g2 is sensitive
        to how many rows participate in violations.
    - Changing the table changes both the violation set and the error. Adding the
        problematic Texas row makes this exact DC fail but repairing or removing those
        records can make the error zero again.

Unexpected violations or large changes after a small data change can indicate a
typo or another data-quality problem. See \"examples/expert/data_cleaning_dc.py\"
for a data-cleaning example that uses DC violations to find records to repair.

Working with these primitives usually requires experimentation: try several
error thresholds and measures, inspect the violating pairs, and compare related
tables when searching for typos or choosing useful parameters.

Related examples worth exploring:
    - DC mining and verification: \"examples/basic/mining_adc.py\" and
        \"examples/basic/verifying_dc.py\".
    - AFD metrics: \"examples/basic/mining_afd.py\" and
        \"examples/basic/verifying_fd_afd.py\".
    - IND/AIND mining and verification: \"examples/basic/mining_ind.py\",
        \"examples/basic/mining_aind.py\", and
        \"examples/basic/verifying_ind_aind.py\".
    - AUCC mining: \"examples/basic/mining_aucc.py\".""")


if __name__ == "__main__":
    main()
