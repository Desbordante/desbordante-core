'''Example 2: 2D parallelepiped, leveling coefficients'''

from tabulate import tabulate
from csv import reader

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
BOLD = '\033[1;37m'
ENDC = '\033[0m'

ENGINE_TEMPS = 'examples/datasets/verifying_pac/engine_temps_bad.csv'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)


print(
    f'''This example illustrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs)
on multiple columns. It continues the first Domain PAC verification example
{CYAN}(examples/basic/verifying_pac/verifying_domain_pac1.py){ENDC}. If you have not read the first part yet,
it is recommended to start there.

In the first example we verified {BLUE}Domain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9{ENDC} on engine temperature sensor readings.
Now, in addition to temperature readings, we also have tachometer data:''')
print(f'{BOLD}{csv_to_str(ENGINE_TEMPS)}{ENDC}')
print()

print(
    f'''The normal operating RPM for this engine is {BLUE}[1500, 3500]{ENDC}. Values outside this range are
not harmful by themselves (as long as they are within {BLUE}[0, 5000]{ENDC}), but:
    * A cold engine may stall at low RPM and can be damaged at high RPM.
    * An overheated engine is especially vulnerable at RPM values outside {BLUE}[1500, 3500]{ENDC}, because
      cooling efficiency depends on RPM.
As in the first example, we use the Domain PAC verifier to check whether the engine operates properly.
''')

print(
    f'''Firstly, we need to create domain. We have a Cartesian product of two segments: {BLUE}[85, 95] x [1500, 3500]{ENDC},
so it would be natural to use parallelepiped.''')
print(
    f'''We now work with two columns: temperature and RPM. The acceptable operating region is a Cartesian product
of two segments:
    * temperature: [85, 95];
    * RPM: [1500, 3500].
This forms a parallelepiped domain: {BLUE}[85, 95] x [1500, 3500]{ENDC}.
''')
# Arguments of generic version are A = (a1, a2, ..., an) and B = (b1, b2, ..., bn).
# Domain is [A, B] = [a1, b1] x [a2, b2] x ... x [an, bn].
parallelepiped = desbordante.pac.domains.Parallelepiped(['85', '1500'],
                                                        ['95', '3500'])

print(
    f'''We run the Domain PAC verifier with the following parameters: domain={BLUE}{parallelepiped}{ENDC},
max_epsilon={BLUE}15{ENDC}.
''')
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=parallelepiped,
               column_indices=[0, 1])
algo.execute(max_epsilon=10)
pac = algo.get_pac()
print(f'Algorithm result: {RED}{pac}{ENDC}.')
print(
    f'A result with δ = {BLUE}{pac.delta}{ENDC} is unexpected. To understand what is happening, we examine the highlights.'
)

print(
    f'''Highlights between {BLUE}0{ENDC} and {BLUE}{pac.epsilon}{ENDC} are: {BOLD}{algo.get_highlights(0, pac.epsilon)}{ENDC}.
''')
print(
    f'''There are very few highlights, which suggests that the parameters may not be chosen correctly.

The question is: what does ε = {BLUE}{pac.epsilon}{ENDC} mean in two-dimensional domain? Should ε correspond to:
    * 10 degrees of temperature difference, or
    * 1500 RPM difference?
To answer this, we need to understand how distance is computed.

The parallelepiped uses the Chebyshev metric to calculate distance between value tuples:
    {BOLD}d(x, y) = max{{|x[1] - y[1]|, ..., |x[n] - y[n]|}}{ENDC}
In our case:
    * temperature differences are on the order of tens;
    * RPM differences are on the order of thousands.
As a result, RPM differences dominate the distance computation, making temperature differences
almost irrelevant. This issue affects all coordinate-wise metric-based domains (currently
Parallelepiped and Ball, though custom domains can be implemented in C++).

To address this, such domains support {CYAN}leveling coefficients{ENDC}, which rescale individual
dimensions. With leveling coefficients, the distance becomes:
    {BOLD}d(x, y) = max{{|x[1] - y[1]| * lc[1], ..., |x[n] - y[n]| * lc[n]}}{ENDC}
To normalize temperatures and RPM scales, we use leveling_coefficients={BLUE}[1, 0.01]{ENDC} parameter.
This treats a 100 RPM difference as comparable to a 1°C difference.

With leveling coefficients applied, we rerun the algorithm.
''')

parallelepiped = desbordante.pac.domains.Parallelepiped(['85', '1500'],
                                                        ['95', '3500'],
                                                        [1, 0.01])
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=parallelepiped,
               column_indices=[0, 1])
algo.execute(max_epsilon=10, min_delta=0.9)
print(f'''Algorithm result: {GREEN}{algo.get_pac()}{ENDC}.
This result is now meaningful and consistent with the findings from the first example.

It is recommended to continue with the third example ({CYAN}examples/basic/verifying_pac/verifying_domain_pac3.py{ENDC}),
which introduces another basic domain type: Ball.''')
