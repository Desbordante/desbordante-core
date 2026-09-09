import desbordante
import pandas as pd
from tabulate import tabulate

class Format:
    BOLD = '\033[1m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    CYAN = '\033[96m'
    MAGENTA = '\033[95m'
    END = '\033[0m'

TABLE = 'examples/datasets/inventory_afd.csv'
ERROR = 0.3

print(f'''
{Format.BOLD}{Format.GREEN}Approximate Functional Dependency (AFD) Discovery Example{Format.END}

{Format.BOLD}{Format.YELLOW}Pattern definition{Format.END}
Functional Dependencies (FDs) are strict constraints where values in the
Left-Hand Side (LHS) columns uniquely determine the Right-Hand Side (RHS)
value. Approximate Functional Dependencies (AFDs) extend this concept by
incorporating error measures, allowing for the discovery of relationships
that hold "mostly" true despite the presence of noise, typos, or outliers.
This approximation uses a threshold (ranging from 0.0 to 1.0), where a
value of 0.0 identifies only perfect, exact dependencies, while higher
values relax the constraints to capture patterns that are nearly consistent
across the dataset. A comprehensive list of all existing measures for AFD
discovery and validation can be found in reference [1].

Desbordante allows your Python programs to discover and validate AFDs
with different measures.

{Format.BOLD}{Format.YELLOW}Supported algorithms{Format.END}
{Format.CYAN}* Pyro{Format.END} [2]: Optimized for speed; supports the {Format.GREEN}g1{Format.END} measure only.
{Format.CYAN}* TANE{Format.END} [3]: Highly versatile; supports all measures coded in Desbordante:
  {Format.GREEN}- g1{Format.END} (fraction of tuple pairs that violate the FD),
  {Format.GREEN}- pdep{Format.END} (probability of equal RHS values given equal LHS values),
  {Format.GREEN}- tau{Format.END} (improvement in predicting the RHS when the LHS is known),
  {Format.GREEN}- mu_plus{Format.END} (pdep corrected for its expected value under random
           permutations),
  {Format.GREEN}- rho{Format.END} (ratio of distinct LHS values to distinct LHS-RHS
       combinations).

For more information consider:
  [1] Marcel Parciak, Sebastiaan Weytjens, Niel Hens, Frank Neven, Liesbet
      M. Peeters, Stijn Vansummeren: {Format.BOLD}{Format.GREEN}Measuring Approximate Functional
      Dependencies: a Comparative Study. {Format.END}CoRR abs/2312.06296 (2023)

  [2] Sebastian Kruse, Felix Naumann: {Format.BOLD}{Format.GREEN}Efficient Discovery of Approximate
      Dependencies{Format.END} Published in PVLDB Vol 11, Issue 7 (2018)

  [3] Yka Huntala, Juha Karkkainen, Pasi Porkka and Hannu Toivonen:
      {Format.BOLD}{Format.GREEN}TANE: An Efficient Algorithm for Discovering Functional and
      Approximate Dependencies{Format.END} The Computer Journal (Oxford University
      Press) (1999)
''')


TABLE_SMALL = 'examples/datasets/inventory_afd.csv'

print(f'''Let’s see how to do this with Desbordante.


{Format.BOLD}{Format.BLUE}-----------------------------------------------------------------
1. Discovery on Small Dataset
-----------------------------------------------------------------{Format.END}
''')

print("Before we begin, consider the following dataset\n"
      f"({Format.CYAN}{TABLE_SMALL}{Format.END}):\n")
df = pd.read_csv(TABLE_SMALL)
print(tabulate(df, headers='keys', showindex=False, tablefmt='psql') + '\n')

# Pyro Discovery
pyro_alg = desbordante.afd.algorithms.Pyro()
pyro_alg.load_data(table=(TABLE_SMALL, ',', True))
pyro_alg.execute(error=ERROR)
result_pyro = pyro_alg.get_fds()

print(f"{Format.BOLD}Let's try Pyro first, with g1 measure and threshold set to 0.3.\nFound AFDs:{Format.END}")
for fd in result_pyro+["..."]:
    print(f"  {Format.GREEN}{fd}{Format.END}")

# Tane Discovery
ERROR_MEASURES = ['g1', 'pdep', 'tau', 'mu_plus', 'rho']
tane_alg = desbordante.afd.algorithms.Tane()
tane_alg.load_data(table=(TABLE_SMALL, ',', True))

print(f"\n{Format.BOLD}Now, let's try different measures with Tane, the threshold stays the same.\nFound AFDs:{Format.END}")
for measure in ERROR_MEASURES:
    tane_alg.execute(error=ERROR, afd_error_measure=measure)
    res = tane_alg.get_fds()
    print(f'  {Format.CYAN}Measure {measure:7}:{Format.END} {len(res)} dependencies.')
    if len(res) > 2:
        res[2:] = ["..."]
    for fd in res:
        print(f"    {Format.GREEN}{fd}{Format.END}")

print(f'''
{Format.BOLD}{Format.YELLOW}Reflections{Format.END}
The discovery results reveal structural patterns in the data. For instance,
the dependency {Format.CYAN}[ProductName] -> Price{Format.END} with a low error suggests that a
product name generally dictates its price, but the non-zero error points to
specific inconsistencies (like the 300 vs 3000 price for a Laptop). Finding
such AFDs is the first step in data profiling: they highlight where your data
follows business logic and where it deviates.

The measures return different sets of dependencies for this dataset even
though the threshold is unchanged. This happens because each measure assigns
a different error value to the same dependency. For example, the
{Format.GREEN}g1{Format.END} error is based on the proportion of tuple pairs that violate the
candidate FD: fewer violating pairs produce a lower {Format.GREEN}g1{Format.END} error. Other
measures quantify deviations using different definitions, so their values are
not directly interchangeable. Each result should be interpreted according to
the selected measure; see [1] for the definitions and comparison.

By correcting the identified price typos in row 3 and row 8, you would see
these AFDs transform into exact FDs (error 0.0), effectively cleaning the
dataset.
''')

print(f'''
{Format.BOLD}{Format.BLUE}-----------------------------------------------------------------
2. Next Steps
-----------------------------------------------------------------{Format.END}''')

print(f'''
After learning about AFD discovery, you may want to explore:
{Format.CYAN}*{Format.END} AFD Verification: Checking if a specific known dependency holds on a
  dataset.
{Format.CYAN}*{Format.END} Exact FD Mining: Finding rules that have zero violations.
{Format.CYAN}*{Format.END} Conditional Functional Dependencies (CFD): Rules that hold only for a
  specific subset of data (e.g., Price depends on Name only for
  'Electronics').
{Format.CYAN}*{Format.END} Association Rules (AR): Finding rules X -> Y between itemsets that
  meet specified support and confidence thresholds.
''')
