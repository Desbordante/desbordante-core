'''Example 1 (advanced): Custom domain'''

from tabulate import tabulate
from csv import reader
from math import sqrt

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
GRAY = '\033[1;30m'
ENDC = '\033[0m'

USER_PREFERENCES = 'examples/datasets/verifying_pac/user_preferences.csv'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)


print(
    f'''{CYAN}This example illustrates the usage of Domain Probabilistic Approximate Constraints (PACs).
Domain PAC on column set X and domain D, with given epsilon and delta means that Pr(x ∈ D±epsilon) ≥ delta.
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network Traffic
Databases" by Filp Korn et al.
If you haven\'t read basic examples on Domain PAC verification (see examples/basic/verifying_pac/ directory),
start with reading them. This example is first in "Advdanced Domain PAC verififcation" series
(see examples/advanced/verifying_pac/ directory).{ENDC}

Consider we have a dataset of user preferences, where user\'s interest on each topic is encoded as
values in [0, 1], where 0 is "not interested at all" and 1 is "very interested":
{GRAY}{csv_to_str(USER_PREFERENCES)}{ENDC}

We need to check if this group of users will be interested in original paper on Domain PACs ("Checks and Balances: ...").
For this purpose, we will represent each user\'s profile as a radius-vector:
     ^ Topic 2
     |
     |   user
     |  x
     | /
     |/    Topic 1
    -+------->
     |

A perfect user has a profile (0.9, 0.4, 0.05), which means "highly interested in Databases,
slightly interested in Networks, not interested in Machine learning".
Let\'s define metric and comparer for our user-vectors.

As a metric we will use cosine distance, which shows magnitude of angle between vectors,
not taking into account vectors\' length:
    {GRAY}dist(x, y) = 1 - cos(angle between x and y) = 1 - (x, y)/(|x| * |y|){ENDC},
where (x, y) is a dot product between x and y.
''')


def cosine_dist(x: list[float], y: list[float]) -> float:
    dot_product = 0
    x_length = 0
    y_length = 0
    for i in range(len(x)):
        dot_product += x[i] * y[i]
        x_length += x[i] * x[i]
        y_length += y[i] * y[i]
    x_length = sqrt(x_length)
    y_length = sqrt(y_length)
    return 1 - dot_product / (x_length * y_length)


print(
    f'''We will compare vectors\' direction, i. e. each vector is associated with a value φ:
    {GRAY}φ(x) = dist(x, (1, 0, 0)) = 1 - (x, (1, 0, 0))/(|x| * |(1, 0, 0)|) = 1 - (x[0])/(|x|){ENDC},
where x[0] is first coordinate of x.
''')


def phi(x: list[float]):
    x_length = 0
    for x_i in x:
        x_length += x_i * x_i
    return 1 - float(x[0]) / x_length


# Arguments are always lists of strings, even when table contains a single conlumn
def compare_angles(x: list[str], y: list[str]) -> bool:
    x_f = [float(x_i) for x_i in x]
    y_f = [float(y_i) for y_i in y]
    return phi(x_f) < phi(y_f)


print(f'''Custom domain is defined by three parameters:
    1. Comparer: a function that takes two value tuples (as lists of strings) and returns {GRAY}True{ENDC}
       when first value is smaller than second.
    2. Distance from domain: a function that takes a value tuple (as list of strings) and returns a distance
       between domain and value.
    3. (Optional) domain name: used to make Domain PAC string representation.

We\'ve already defined comparer. Distance from domain will be dist(x, (0.9, 0.4, 0.05)), and name will be {BLUE}"(0.9, 0.4, 0.05)"{ENDC}.
''')

PERFECT_USER = [0.9, 0.4, 0.05]


# Argument is always a list of strings
def dist_from_domain(x: list[str]) -> float:
    x_f = [float(x_i) for x_i in x]
    return cosine_dist(x_f, PERFECT_USER)


domain = desbordante.pac.domains.CustomDomain(compare_angles, dist_from_domain,
                                              "(0.9, 0.4, 0.05)")

print(
    f'Let\'s run Domain PAC verifier with domain={BLUE}{domain}{ENDC}, max_epsilon={BLUE}0.5{ENDC}.'
)
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(USER_PREFERENCES, ',', True),
               domain=domain,
               column_indices=[0, 1, 2])
algo.execute(max_epsilon=0.5)
pac_1 = algo.get_pac()
print(f'''Algorithm result: {GREEN}{pac_1}{ENDC}.

Now let\'s set min_delta to {BLUE}0.6{ENDC}.''')
algo.execute(max_epsilon=0.5, min_delta=0.6)
pac_2 = algo.get_pac()
print(f'''Algorithm result: {GREEN}{pac_2}{ENDC}.

This means that all users will be a bit interested in the paper (perfect user±{GREEN}{pac_1.epsilon:.3f}{ENDC}),
but only {GREEN}{pac_2.delta * 100:.0f}%{ENDC} of users will be highly interested (perfect user±{GREEN}{pac_2.epsilon:.3f}{ENDC}).

Note: highlights can be used to determine who will be less and who will be more interested.
See {CYAN}examples/basic/verifying_pac/verifying_domain_pac1.py{ENDC}.''')

# C++ note: Custom domain is called "Untyped domain" in C++ code, becuase it erases type
# information, converting all values to strings. If you use C++ library, it's recommended to
# implement IDomain interface or derive from MetricBasedDomain (if your domain is based on
# coordinate-wise metrics). See Parallelepiped and Ball implementations as examples.
