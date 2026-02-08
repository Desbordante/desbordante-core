'''Example 3: 2D ball'''

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
    f'''This example demonstrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs).
It is the third example of "Basic Domain PAC verification" series (see the {CYAN}examples/basic/verifying_pac/{ENDC} directory).
If you haven\'t read first and second parts yet, it is recommended to start there.

In the first example we verified the following Domain PAC on temperature sensor readings:
    {BLUE}Domain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9{ENDC}
In the second example, we added tachometer readings and validated a Domain PAC on two columns using
the Parallelepiped domain.

The sensor readings are the same as before:
{BOLD}{csv_to_str(ENGINE_TEMPS)}{ENDC}
''')

print(
    f'''The parallelepiped {BLUE}[{{85, 1500}}, {{95, 3500}}]{ENDC} is a rectangle in the (temperature, RPM) space:
    (95, 1500)      (95, 3500)
            +--------+
            |        |
            +--------+
    (85, 1500)      (85, 3500)
Our task from the second example was:
    The normal operating RPM for this engine is {BLUE}[1500, 3500]{ENDC}. Values outside this range are
    not harmful by themselves (as long as they are within {BLUE}[0, 5000]{ENDC}), but:
        * A cold engine may stall at low RPM and can be damaged at high RPM.
        * An overheated engine is especially vulnerable at RPM values outside {BLUE}[1500, 3500]{ENDC}, because
          cooling efficiency depends on RPM.

A rectangle does not perfectly describe these conditions. For example,
    * (80, 3900) is very harmful,
    * (80, 1600) is mostly acceptable.
However, both points have the same distance from the rectangle boundary. This shows that a shape
with sharp corners does not model the risk accurately.
What we rally want is a smooth shape without corners -- an ellipse.

In this approach, ellipses (and their higher-dimensional equivalents) are represented by the Ball domain.
You might wonder: a ball has the same radius in all dimensions, while an ellipse has different ones.
The answer is leveling coefficients.

In metric-space terms, a ball is defined as {BLUE}B = {{x : dist(x, center) < r}}.
The Ball domain uses the Euclidean metric:
    {BOLD}dist(x, y) = sqrt((x[1] - y[1])^2 * lc[1] + ... + (x[n] - y[n])^2 * lc[n]){ENDC}
Here, lc is the list of leveling coefficients, introduced in the second example. They allow us to
scale dimensions differently -- effectively turning a circle into an ellipse.

To balance temperature and RPM scales, we use levelling_coefficients={BLUE}[1, 0.005]{ENDC}.
This treats a 200 RPM difference as roughly equivalent to a 1°C difference.
''')

# Formally, it's a disk: D = {x : dist(x, center) ≤ r}, but name "ball" is more clear
ellipse = desbordante.pac.domains.Ball(["90", "2500"], 5, [1, 0.005])

print(f'We now run the Domain PAC verifier with domain={BLUE}{ellipse}{ENDC}.')
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=ellipse,
               column_indices=[0, 1])
algo.execute()
print(f'''Algorithm result:
    {GREEN}{algo.get_pac()}{ENDC}''')

parallelepiped = desbordante.pac.domains.Parallelepiped(['85', '1500'],
                                                        ['95', '3500'],
                                                        [1, 0.005])
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=parallelepiped,
               column_indices=[0, 1])
algo.execute()

print(f'''For comparison, the Parallelepiped domain previously produced:
    {RED}{algo.get_pac()}{ENDC}
''')
print(
    f'''Although the numerical values differ slightly, the Ball domain better reflects the actual operating
conditions, because it models gradual risk changes instead of sharp rectangular boundaries.

It is recommended to continue with the fourth example ({CYAN}examples/basic/verifying_pac/verifying_domain_pac3.py{ENDC}),
which demonstrates another practical usage of the Ball domain.''')
