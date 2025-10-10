'''Example 3: 2D ball'''

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
This example is third of "Basic Domain PAC verification" series (see examples/basic/verifying_pac/ directory).
If you haven\'t read first and second parts, start with reading them.{ENDC}

In first example we\'ve verified {BLUE}Domain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9{ENDC} on temperature sensor readings.
In the second part ve\'we added tachometer readings and validated {BLUE}Domain PAC Pr(x ∈ [{{85, 1500}}, {{95, 3500}}]±15) ± 0.85{ENDC}.
In both cases we\'ve used Parallelepiped domain. Here are sensor readings:
{GRAY}{csv_to_str(ENGINE_TEMPS)}{ENDC}
''')

print(
    f'''Parallelepiped {BLUE}[{{85, 1500}}, {{95, 3500}}]{ENDC} is a rectangle like this:
    (95, 1500)      (95, 3500)
            +--------+
            +--------+
    (85, 1500)      (85, 3500)
Here is our task from second example:
    Normal temperatures lie in {BLUE}[85, 95]{ENDC}. Normal RPM for our engine lie in {BLUE}[1500, 3500]{ENDC}.
    Values in {BLUE}[85, 95]{ENDC}°C and in {BLUE}[0, 5000]{ENDC} RMP are not harmful themselves, but
    cold engine will stall on low RPM and can be damaged on high RPM, and overheated engine can blow up,
    especially on RPM outside of {BLUE}[1500, 3500]{ENDC} (because cooling system operation depends on RPM).

You may have noticed that a rectangle doesn\'t describe our task perfeclty: value like {BLUE}(80, 3900){ENDC}
is very harmful for the engine, while {BLUE}(80, 1600){ENDC} is quite OK, but they have the same distance from rectangle.
Wish there was a geometrical shape without corners... Wait. It\'s ellipse!

Ellipses of any arities are described by Ball domain. You may ask: "ball has the same radii in all dimensions,
while ellipse has different ones. How can I describe ellipse using Ball?". Don\'t worry: ball (in terms
of metric spaces) is B = {{x : dist(x, center) < r}}. Ball domain uses Euclidean metric:
dist(x, y) = sqrt((x[0] - y[0])^2 * lc[0] + ... + (x[n] - y[n])^2 * lc[n])
lc is a list of levelling coefficients, described in second Domain PAC example. We will use them to
turn a circle into an ellipse.
''')

# Formally, it's a disk: D = {x : dist(x, center) ≤ r}, but name "ball" is more clear
ellipse = desbordante.pac.domains.Ball(["90", "2500"], 10, [2, 0.01])

print(
    f'Let\'s run Domain PAC verifier with the following parameters: domain={BLUE}{ellipse}{ENDC}, max_epsilon={BLUE}10{ENDC}, min_delta={BLUE}0.85{ENDC}.'
)
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS, ',', True),
               domain=ellipse,
               column_indices=[0, 1])
algo.execute(max_epsilon=15, min_delta=0.85)
print(f'Algorithm result: {GREEN}{algo.get_pac()}{ENDC}.')

print(
    f'The result with rectangle domain was {RED}Domain PAC Pr(x ∈ [{85, 1500}, {95, 3500}]±10) ≥ 0.863636 on columns [t rpm]{ENDC}.'
)
print(
    f'''You can see that PAC with ball domain better reflects given conditions.

Now it\'s recommended to continue with reading fourth example ({CYAN}examples/basic/verifying_pac/verifying_domain_pac3.py{ENDC}),
which demonstrates another usage of Ball domain.''')
