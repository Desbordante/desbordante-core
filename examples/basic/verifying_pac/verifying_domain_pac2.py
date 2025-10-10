'''Example 2: 2D parallelepiped, leveling coefficients'''

from tabulate import tabulate
from csv import reader

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
GRAY = '\033[1;30m'
ENDC = '\033[0m'

ENGINE_TEMPS = 'examples/datasets/verifying_pac/engine_temps_bad.csv'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)


print(
    f'''{CYAN}This example illustrates the usage of Domain Probabilistic Approximate Constraints (PACs).
This example continues first Domain PAC verification example (examples/basic/verifying_pac/verifying_domain_pac1.py).
If you haven't read first part, start with reading it.{ENDC}
''')

print(
    f'''In first example we\'ve verified {BLUE}Domain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9{ENDC} on temperature sensor readings.
Now we also have tachometer readings:''')
print(f'{GRAY}{csv_to_str(ENGINE_TEMPS)}{ENDC}')
print()

print(
    f'''Normal RPM for our engine lie in {BLUE}[1500, 3500]{ENDC}. Values in {BLUE}[0, 5000]{ENDC} are not harmful themselves, but
cold engine will stall on low RPM and can be damaged on high RPM, and overheated engine can blow up, especially on RPM
outside of {BLUE}[1500, 3500]{ENDC} (because cooling system operation depends on RPM).
Let\'s use Domain PAC verifier to check if engine works properly, just like we did in first example.
''')

print(
    f'''Firstly we need to create domain. We have a cartesian product of two segments: {BLUE}[85, 95] x [1500, 3500]{ENDC},
so it would be natural to use parallelepiped.''')
# Arguments of generic version are A = (a0, a1, ..., an) and B = (b0, b1, ..., bn).
# Domain is [A, B] = [a0, b0] x [a1, b1] x ... x [an, bn].
parallelepiped = desbordante.pac.domains.Parallelepiped(['85', '1500'],
                                                        ['95', '3500'])

print(
    f'Let\'s run Domain PAC verifier with domain={BLUE}{parallelepiped}{ENDC}, max_epsilon={BLUE}15{ENDC}, min_delta={BLUE}0.85{ENDC}.'
)
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=parallelepiped,
               column_indices=[0, 1])
algo.execute(max_epsilon=15, min_delta=0.85)
pac = algo.get_pac()
print(f'Algorithm result: {RED}{pac}{ENDC}.')
print(
    'Delta is less than min_delta? What does it mean? Let\'s look at highlights:'
)

# Default values are eps_1 = min_epsilon and eps_2 = pac.epsilon
print(
    f'Highlights between {BLUE}0{ENDC} and {BLUE}{pac.epsilon}{ENDC}: {algo.get_highlights()}\n'
)
print(f'''There are no highlights! Maybe we\'ve selected wrong parameters?
Which epsilon should we use: {BLUE}10{ENDC} for temperatures or {BLUE}1500{ENDC} for RPM? We\'ll need some math to answer this.

Parallelepiped uses Chebyshev metric to calculate distance between value tuples: {GRAY}d(x, y) = max{{|x[0] - y[0]|, ..., |x[n] - y[n]|}}{ENDC}.
But in our case difference between RPM values will always be greater than difference between temperatures!
For such situations all coodinate-wise-metric-based domains (currently it's Parallelepiped and Ball, but you can
define your own in C++) have additional parameter: list of levelling coefficients. When it\'s passed,
distance becomes {GRAY}d(x, y) = max{{|x[0] - y[0]| * lc[0], ..., |x[n] - y[n]| * lc[n]}}{ENDC}.

Let\'s use levelling_coefficients={BLUE}[1, 0.01]{ENDC} to normalize temperatures and RPM.'''
      )
# Also parallelepiped uses product compare: x < y iff x[0] < y[0] & ... & x[n] < y[n]
parallelepiped = desbordante.pac.domains.Parallelepiped(['85', '1500'],
                                                        ['95', '3500'],
                                                        [1, 0.01])
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=parallelepiped,
               column_indices=[0, 1])
algo.execute(max_epsilon=15, min_delta=0.85)
print(f'''Algorithm result: {GREEN}{algo.get_pac()}{ENDC}.

Now it\'s recommended to continue with reading third example ({CYAN}examples/basic/verifying_pac/verifying_domain_pac3.py{ENDC}),
which introduces another basic domain: Ball.''')
