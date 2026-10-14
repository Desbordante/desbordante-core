from time import perf_counter

import desbordante
import pandas as pd
from tabulate import tabulate


class Format:
    BOLD = '\033[1m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    CYAN = '\033[96m'
    END = '\033[0m'


TABLE = 'examples/datasets/inventory_afd.csv'
ERROR = 0.3
ERROR_MEASURES = ['g1', 'pdep', 'tau', 'mu_plus', 'rho']
COMPARISON_TABLE = 'examples/datasets/adult.csv'
COMPARISON_COLUMN_COUNT = 11
COMPARISON_ERROR = 0.0
COMPARISON_RUNS = 3


def print_mining_results(
    algorithm,
    results,
    threshold,
    max_results=None,
    show_dependency_errors=False,
):
    print(
        f'{Format.BOLD}{Format.YELLOW}Found AFDs by '
        f'{Format.CYAN}{algorithm}{Format.END}:'
    )
    for index, (measure, dependencies) in enumerate(results):
        if index:
            print()
        print(
            f'  {Format.BOLD}Measure:{Format.END} {Format.GREEN}{measure}{Format.END}'
            f' | {Format.BOLD}Threshold:{Format.END} {threshold}'
            f' | {Format.BOLD}Found:{Format.END} {len(dependencies)} AFDs'
        )
        displayed_dependencies = dependencies[:max_results]
        for dependency in displayed_dependencies:
            error = ''
            if show_dependency_errors:
                error = (
                    f' | {Format.BOLD}Error:{Format.END} '
                    f'{Format.YELLOW}{dependency.get_threshold():.4f}{Format.END}'
                )
            print(f'    {Format.GREEN}{dependency}{Format.END}{error}')
        if max_results is not None and len(dependencies) > max_results:
            print(f'    {Format.CYAN}...{Format.END}')


def measure_average_execution_time(algorithm, runs, **execute_options):
    execution_times = []
    for _ in range(runs):
        start = perf_counter()
        algorithm.execute(**execute_options)
        execution_times.append(perf_counter() - start)
    return sum(execution_times) / len(execution_times)


print(f'''
{Format.BOLD}{Format.GREEN}Approximate Functional Dependency (AFD) Discovery Example{Format.END}

{Format.BOLD}{Format.YELLOW}Introduction{Format.END}

The definitions and explanations in this example are based on the
following works:

{Format.BOLD}{Format.YELLOW}References{Format.END}
  [1] Marcel Parciak, Sebastiaan Weytjens, Niel Hens,
      Frank Neven, Liesbet M. Peeters, Stijn Vansummeren:
      {Format.BOLD}{Format.GREEN}Measuring Approximate Functional Dependencies:
      a Comparative Study.{Format.END} CoRR abs/2312.06296 (2023)

  [2] Sebastian Kruse, Felix Naumann: Efficient Discovery
      of {Format.BOLD}{Format.GREEN}Approximate Dependencies.{Format.END} Published in PVLDB
      Vol 11, Issue 7 (2018)

  [3] Ykä Huhtala, Juha Kärkkäinen, Pasi Porkka and
      Hannu Toivonen: {Format.BOLD}{Format.GREEN}TANE: An Efficient Algorithm for
      Discovering Functional and Approximate Dependencies.{Format.END}
      The Computer Journal (Oxford University Press) (1999)

{Format.BOLD}{Format.YELLOW}Functional Dependencies{Format.END}
A functional dependency is a rule X -> Y according to which the values
of the attributes on the left-hand side (LHS, X) uniquely determine the
values of the attributes on the right-hand side (RHS, Y). In other words,
if we know the values of the columns on the left-hand side, we know exactly
what values are in the columns on the right-hand side.

An approximate functional dependency is a generalization of a
functional dependency. An AFD measure indicates how closely the
dependency resembles an exact functional dependency. When the measure
value is high, the values on the left-hand side allow us to determine
the values of the columns on the right-hand side with a high degree of
certainty.

{Format.BOLD}{Format.YELLOW}AFD Measures{Format.END}
All AFD measures report the value in the [0, 1] range, where 1
corresponds to an exact functional dependency and 0 corresponds to the
greatest degree of deviation from it. Currently, the project supports
the following AFD measures:
  {Format.GREEN}- g1{Format.END} (fraction of tuple pairs that do not violate the FD),
  {Format.GREEN}- pdep{Format.END} (probability of equal RHS values given equal
    LHS values),
  {Format.GREEN}- tau{Format.END} (improvement in predicting the RHS when the LHS
    is known),
  {Format.GREEN}- mu_plus{Format.END} (pdep corrected for its expected value under
    random permutations),
  {Format.GREEN}- rho{Format.END} (ratio of distinct LHS values to distinct
    LHS-RHS combinations).

{Format.BOLD}{Format.YELLOW}AFD Discovery Algorithms{Format.END}
Desbordante supports AFD discovery and verification, and this example
focuses on discovery. Several algorithms are available for this task;
from a user’s perspective, they differ mainly in performance and the
AFD measures they support.

The core AFD search algorithms in Desbordante are Pyro and Tane. In terms
of the characteristics mentioned, they can be briefly described as
follows: Pyro is a fast AFD mining algorithm that supports only the g1
error measure; Tane is a slower algorithm that supports all error
measures available in Desbordante.

Let’s now move on to an example of AFD mining.

{Format.BOLD}{Format.BLUE}----------------------------------------------------------------------
1. Using Different Error Measures to Mine AFDs
----------------------------------------------------------------------{Format.END}

{Format.BOLD}{Format.YELLOW}Dataset{Format.END}
For this AFD mining example, we will use the following dataset:
{Format.CYAN}{TABLE}{Format.END}
''')

df = pd.read_csv(TABLE)
print(tabulate(df, headers='keys', showindex=False, tablefmt='psql') + '\n')

print(f'''{Format.BOLD}{Format.YELLOW}Pyro with g1{Format.END}
Let’s start with the default search algorithm --- Pyro. To run the
algorithm, you must specify an error threshold. It limits the discovery
results to dependencies with error values that do not exceed the
specified threshold. An error measure is defined as `1 - AFD measure`.

We will use a threshold value of 0.3. This way, we allow approximate
dependencies that deviate only slightly from exact FDs.
''')

pyro_alg = desbordante.afd.algorithms.Pyro()
pyro_alg.load_data(table=(TABLE, ',', True))
pyro_alg.execute(error=ERROR)
result_pyro = pyro_alg.get_fds()

print_mining_results('Pyro', [('g1', result_pyro)], ERROR)

print(f'''
{Format.BOLD}{Format.YELLOW}Pyro Results{Format.END}
As we can see, the algorithm found six AFDs. Based on the results, we
can make the following observations: knowing the ProductName, we can
determine the Price of the item with a high degree of certainty, and
vice versa.

AFDs can aid in a variety of data quality tasks [1-3]. In this dataset,
for example, one Laptop has a Price of 300, whereas the other Laptop
entries have a Price of 3000. This inconsistency may indicate a
data-entry error. FD and AFD discovery and verification can help detect
such cases automatically.

An example of using AFDs to identify potential data errors can be found
here:
    {Format.CYAN}examples/expert/mine_typos.py{Format.END}

{Format.BOLD}{Format.YELLOW}Tane with Multiple Measures{Format.END}
As mentioned in the introduction, there are many ways to measure the
deviation of AFDs from exact FDs, each of which reflects a specific
characteristic of the relationship. Therefore, the choice of AFD
measure should be based on the task requirements. Now let’s consider
discovering AFDs using the other available AFD measures in
Desbordante. To do this, we need to use the Tane algorithm. In addition
to the threshold value, we must provide the error_measure parameter,
which determines the AFD measure used: ‘g1’, ‘pdep’, ‘tau’, ‘mu_plus’,
or ‘rho’.

Unlike Pyro, Tane also reports the error value of each discovered
AFD. This value is shown next to the corresponding dependency below.
''')

tane_alg = desbordante.afd.algorithms.Tane()
tane_alg.load_data(table=(TABLE, ',', True))
tane_results = []

for measure in ERROR_MEASURES:
    tane_alg.execute(error=ERROR, afd_error_measure=measure)
    result_tane = tane_alg.get_fds()
    tane_results.append((measure, result_tane))

print_mining_results(
    'Tane',
    tane_results,
    ERROR,
    max_results=2,
    show_dependency_errors=True,
)

print(f'''
{Format.BOLD}{Format.YELLOW}Comparing Error Measures{Format.END}
As you can see, depending on the method used to calculate the error, we
obtain different numbers of AFDs. Therefore, it is important to choose
an error measure based on the task requirements. A larger number of AFDs
found does not always mean a better result: some of them may be
uninformative. The meaning and definition of each error measure are
discussed in detail in [1]. Also, keep in mind that if a particular
measure allows you to find more AFDs, this does not mean that it allows
you to find all AFDs detected by another measure: the resulting sets are
not necessarily subsets of one another. The most you can hope for in
such a case is the detection of all minimal [1] exact FDs.

Let’s now compare the mining algorithms rather than the error measures.
''')

comparison_data = pd.read_csv(
    COMPARISON_TABLE,
    sep=';',
    header=None,
    usecols=range(COMPARISON_COLUMN_COUNT),
)

comparison_pyro = desbordante.afd.algorithms.Pyro()
comparison_pyro.load_data(table=comparison_data)
pyro_time = measure_average_execution_time(
    comparison_pyro,
    COMPARISON_RUNS,
    error=COMPARISON_ERROR,
)

comparison_tane = desbordante.afd.algorithms.Tane()
comparison_tane.load_data(table=comparison_data)
tane_time = measure_average_execution_time(
    comparison_tane,
    COMPARISON_RUNS,
    error=COMPARISON_ERROR,
    afd_error_measure='g1',
)

if pyro_time < tane_time:
    faster_algorithm, slower_algorithm = 'Pyro', 'Tane'
    speedup = tane_time / pyro_time
    pyro_time_color, tane_time_color = Format.GREEN, Format.YELLOW
else:
    faster_algorithm, slower_algorithm = 'Tane', 'Pyro'
    speedup = pyro_time / tane_time
    pyro_time_color, tane_time_color = Format.YELLOW, Format.GREEN

comparison_results = [
    [
        f'{Format.CYAN}Pyro{Format.END}',
        f'{Format.GREEN}g1{Format.END}',
        f'{pyro_time_color}{pyro_time:.3f} s{Format.END}',
    ],
    [
        f'{Format.CYAN}Tane{Format.END}',
        f'{Format.GREEN}g1{Format.END}',
        f'{tane_time_color}{tane_time:.3f} s{Format.END}',
    ],
]

print(f'''{Format.BOLD}{Format.BLUE}----------------------------------------------------------------------
2. Comparison of Mining Algorithms
----------------------------------------------------------------------{Format.END}

{Format.BOLD}{Format.YELLOW}Benchmark Setup{Format.END}
As mentioned earlier, Pyro is faster than Tane but is a less flexible
algorithm. Let’s compare their execution times.

{Format.BOLD}Dataset:{Format.END} {Format.CYAN}{COMPARISON_TABLE}{Format.END}
{Format.BOLD}Columns used:{Format.END} {COMPARISON_COLUMN_COUNT}
''')

print(tabulate(
    comparison_results,
    headers=['Algorithm', 'Measure', 'Average time'],
    tablefmt='psql',
    stralign='center',
    numalign='center',
    disable_numparse=True,
))

print(f'''
{Format.BOLD}{Format.YELLOW}Benchmark Results{Format.END}
On this dataset, {faster_algorithm} was {speedup:.2f} times faster than {slower_algorithm} on average.
The reported values are averages over three runs and exclude data loading.

However, these results should not be treated as universal. Execution
time depends on the dataset, the selected threshold, and the available
hardware. Pyro is designed specifically for the g1 error measure,
whereas Tane is more flexible and supports all error measures available
in Desbordante. Therefore, the algorithm should be selected based not
only on speed, but also on the measure required for the task.

{Format.BOLD}{Format.BLUE}----------------------------------------------------------------------
3. Recommendations for Further Study
----------------------------------------------------------------------{Format.END}

After studying this example, we recommend reviewing the following
materials to deepen your understanding:
{Format.CYAN}*{Format.END} AFD Measures Interpretation: Explaining various AFD measures.
    {Format.CYAN}examples/advanced/afd_measures_interpretation.py{Format.END}

{Format.CYAN}*{Format.END} Exact FD Mining: Finding rules that have zero violations.
    {Format.CYAN}examples/basic/mining_fd.py{Format.END}

{Format.CYAN}*{Format.END} Conditional Functional Dependencies (CFD): Rules that hold
  only for a specific subset of data (e.g., Price depends on
  Name only for 'Electronics').
    {Format.CYAN}examples/basic/verifying_cfd.py{Format.END}

{Format.CYAN}*{Format.END} Dynamic Dependency Verification: Re-evaluating known FDs
  and AFDs while the underlying dataset changes.
    {Format.CYAN}examples/basic/dynamic_verifying_fd.py{Format.END}
    {Format.CYAN}examples/basic/dynamic_verifying_afd.py{Format.END}

{Format.CYAN}*{Format.END} Approximate FD Discovery: Using faster approximate algorithms
  to discover exact FDs in large datasets.
    {Format.CYAN}examples/basic/mining_fd_approximate.py{Format.END}

{Format.CYAN}*{Format.END} Matching Dependencies (MD): Discovering dependencies in dirty
  data using a configurable similarity measure for each attribute.
    {Format.CYAN}examples/basic/mining_md.py{Format.END}

{Format.CYAN}*{Format.END} Differential Dependencies (DD): Working with dirty data by
  defining acceptable value differences for individual attributes.
    {Format.CYAN}examples/basic/mining_dd.py{Format.END}

{Format.CYAN}*{Format.END} Probabilistic Functional Dependencies (PFD): An AFD variant
  with dedicated error measures.
    {Format.CYAN}examples/basic/mining_pfd.py{Format.END}
    {Format.CYAN}examples/basic/verifying_pfd.py{Format.END}

{Format.CYAN}*{Format.END} Soft Functional Dependencies (SFD): An AFD variant based on
  the rho measure.
    {Format.CYAN}examples/basic/mining_sfd.py{Format.END}
''')
