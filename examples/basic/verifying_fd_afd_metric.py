import csv
from typing import List, Any

import desbordante

# --- Standard console formatting (ANSI escape codes) ---
class Format:
    BOLD = '\033[1m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    CYAN = '\033[96m'
    END = '\033[0m'

def print_table(headers: List[str], rows: List[List[Any]], align: str = "left"):
    """
    Helper for printing aligned tables without third-party libraries.
    Automatically adjusts column widths to the longest value.
    """
    str_rows = [[str(item) for item in row] for row in rows]
    str_headers = [str(h) for h in headers]

    widths = [max(len(h), max((len(r[i]) for r in str_rows), default=0))
              for i, h in enumerate(str_headers)]

    align_char = "<" if align == "left" else ">"

    header_row = " | ".join(f"{h:{align_char}{w}}" for h, w in zip(str_headers, widths))
    separator = "-+-".join("-" * w for w in widths)

    print(f"    {header_row}")
    print(f"    {separator}")
    for row in str_rows:
        print(("    " + " | ".join(f"{item:{align_char}{w}}" for item, w in zip(row, widths))).rstrip())


def calculate_metric(alg: Any, lhs: List[int], rhs: List[int], metric: str) -> float:
    alg.execute(lhs_indices=lhs, rhs_indices=rhs, metric=metric)
    return alg.get_result()


# --- 1. DEFINITION & CITATION ---
print(f'''
{Format.BOLD}{Format.GREEN}Approximate Functional Dependency (AFD) Metric Verification Example{Format.END}

{Format.BOLD}{Format.YELLOW}Understanding dependencies and metrics.{Format.END}
A Functional Dependency (FD) is a rule of the form LHS -> RHS
(Left-Hand Side -> Right-Hand Side). It states that if two records
have the same values in the LHS columns (e.g., Product Name), then
they must have the same values in the RHS columns (e.g., Price).

However, real-world data often contains typos, outliers, or
other types of inconsistencies. An Approximate Functional
Dependency (AFD) allows such rules to hold "mostly" true. The
error metric is a score that quantifies the degree of violation:
- 0.0 means the rule is perfect (an exact dependency).
- Values closer to 1.0 indicate that the relationship is weak or non-existent.

We say that an approximate functional dependency holds if the computed
value of an error metric is less than a pre-specified error threshold.

To quantify these violations, Desbordante utilizes several error metrics,
each offering a different mathematical lens on the relationship:
- {Format.BOLD}{Format.GREEN}g2{Format.END}: Measures the fraction of tuples that do not participate in any violating pair;
- {Format.BOLD}{Format.GREEN}tau{Format.END}: Measures the proportional reduction in logical entropy of the RHS given
  the LHS;
- {Format.BOLD}{Format.GREEN}mu_plus{Format.END}: A logical-entropy metric that corrects for permutation bias;
- {Format.BOLD}{Format.GREEN}fi{Format.END}: Represents the reduction of Shannon entropy (uncertainty) of the RHS
  achieved by knowing the LHS.

Metric definitions are taken from the following paper: Marcel
Parciak, Sebastiaan Weytjens, Niel Hens, Frank Neven, Liesbet
M. Peeters, Stijn Vansummeren: {Format.BOLD}{Format.GREEN}Measuring Approximate Functional
Dependencies: a Comparative Study. {Format.END}CoRR abs/2312.06296 (2023)

In this example we demonstrate how to verify specific dependency candidates,
interpret various error metrics to assess data quality, and utilize
internal optimizations for processing large-scale datasets efficiently.
''')

# --- 2. BASIC USAGE ---
print(f'''
{Format.BOLD}{Format.BLUE}-----------------------------------------------------------------
1. Basic usage: dataset profiling
-----------------------------------------------------------------{Format.END}

To demonstrate the pattern, we check a common hypothesis: does a person's occupation
('Occupation', col 7) determine their income bracket ('Salary', col 15)?
''')

DATA_PATH = 'examples/datasets/adult.csv'

alg = desbordante.afd_metric_calculation.algorithms.Default()
print(f"Loading dataset: {DATA_PATH} ...")
alg.load_data(table=(DATA_PATH, ';', False))
print("Data loaded successfully.\n")

with open(DATA_PATH, newline='') as f:
    raw_rows = [row for _, row in zip(range(10), csv.reader(f, delimiter=';'))]
show_cols = [0, 1, 6, 9, 14]
raw_headers = ['Age', 'Work Sector', 'Occupation', 'Sex', 'Salary']

# Insert "..." separators to indicate hidden columns between shown columns
headers = []
col_indices = []
for i, (h, c) in enumerate(zip(raw_headers, show_cols)):
    if i > 0:
        headers.append('...')
        col_indices.append(None)
    headers.append(h)
    col_indices.append(c)

table_rows = []
for row in raw_rows:
    display_row = []
    for c in col_indices:
        if c is None:
            display_row.append('...')
        else:
            display_row.append(row[c])
    table_rows.append(display_row)

# Bottom row to show the table continues
table_rows.append(['...'] * len(headers))

print(f"{Format.BOLD}First 10 rows of the dataset (showing only a subset of columns):{Format.END}")
print_table(headers, table_rows, align="left")
print()

alg.execute(lhs_indices=[6], rhs_indices=[14], metric='mu_plus')
error_result = alg.get_result()
print(f"{Format.GREEN}Result for verifying AFD [Occupation] -> [Salary] (mu_plus metric): {error_result:.6f}{Format.END}\n")

print(f'''
{Format.BOLD}{Format.YELLOW}Interpreting the result.{Format.END}
An error of ~0.12 suggests that the 'Occupation' attribute is a highly influential
factor, but it does not strictly determine 'Salary'. In practical terms, this value
tells us that while most people in a specific occupation fall into the same salary
bracket, there are roughly 12% "exceptions" or noise in the functional relationship.

{Format.BOLD}{Format.YELLOW}The need for multiple metrics.{Format.END}
Evaluating a dependency with a single metric can sometimes be misleading. By comparing
different mathematical approaches, we can verify the stability of the relationship:

''')

metrics_headers = ["Metric", "Value"]
metrics_rows = []

for metric in ['g2', 'tau', 'mu_plus', 'fi']:
    val = calculate_metric(alg, [6], [14], metric)
    metrics_rows.append([metric, f"{val:.3f}"])

print_table(metrics_headers, metrics_rows, align="right")
print()

print(f'''
Here, g2 reports a 1.0 (total violation), suggesting no dependency exists. This is
because g2 is a strict, pair-wise metric: if even a few identical LHS values are paired
with different RHS values across the dataset, g2 penalizes the relationship heavily.

Conversely, entropy-based metrics (tau, mu_plus, fi) hover around 0.12. These metrics
measure "information flow"—they show that even if the rule isn't perfect, knowing the
'Occupation' reduces our uncertainty about 'Salary' by nearly 88%. This suggests a
genuine structural pattern exists, even though it fails the strict pair-wise consistency
test used by g2. This highlights why experimenting with multiple metrics is vital for
accurate data profiling.
''')

# --- 3. MULTI-COLUMN LHS ---
print(f'''
{Format.BOLD}{Format.BLUE}-----------------------------------------------------------------
2. Multi-column LHS
-----------------------------------------------------------------{Format.END}

In many scenarios, a single attribute is insufficient to define a rule. Here, we
investigate a more complex relationship in the same dataset: Does the combination of
Occupation (col 7) and Sex (col 10) determine Salary (col 15)?

The pattern also supports complex determinants. Multiple LHS columns are automatically
intersected in the C++ core to form a composite index.
''')

multi_column_rows = []
for metric in ['tau', 'mu_plus', 'g2']:
    val = calculate_metric(alg, [6, 9], [14], metric)
    multi_column_rows.append([metric, f"{val:.3f}"])

print(f"   {Format.CYAN}Case: Occupation + Sex -> Salary{Format.END}")
print_table(metrics_headers, multi_column_rows, align="right")
print()

print(f'''
This approach allows you to identify hidden constraints that only emerge when attributes
are analyzed in combination. Generally, as more relevant context is added to the LHS,
the dependency becomes more `exact*`, leading to lower error values. Multi-column
verification is therefore a crucial step in refining data quality rules beyond simple
single-attribute checks.

* Keep in mind that this does not always indicate a `true` dependency. Sometimes this
effect can arise simply from insufficient data, because violating a composite dependency
requires substantially more data to find a counterexample. Which exact case applies to
your result has to be checked; it depends on the specific dependency, the table
attributes, and its size.
''')

# --- 4. EXCEPTIONS & NEXT STEPS ---
print(f'''
{Format.BOLD}{Format.YELLOW}Working with exceptions.{Format.END}
Beyond measuring aggregate error, Desbordante can also highlight individual cluster-level
violations — groups of rows sharing the same LHS value but differing on the RHS. This
helps distinguish data-entry typos from genuine structural patterns. A dedicated
walkthrough is available at:
    {Format.GREEN}examples/basic/verifying_fd_afd.py{Format.END}
''')

print(f'''
{Format.BOLD}{Format.BLUE}-----------------------------------------------------------------
3. Next steps
-----------------------------------------------------------------{Format.END}''')

print(
f'''
- Explore AFD Discovery to automatically find these dependencies.
    {Format.GREEN}examples/basic/mining_afd.py{Format.END}
- See Exact FD Mining for rules that hold with zero errors.
    {Format.GREEN}examples/basic/mining_fd.py{Format.END}
- Inspect FD and AFD exceptions with cluster analysis.
    {Format.GREEN}examples/basic/verifying_fd_afd.py{Format.END}
- Check Conditional Functional Dependencies (CFD) for context-specific rules.
    {Format.GREEN}examples/basic/mining_cfd.py{Format.END}
''')
