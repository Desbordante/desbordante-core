'''Example 1 (advanced): Custom domain'''

from tabulate import tabulate
from csv import reader
from math import sqrt

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
BOLD = '\033[1;37m'
ENDC = '\033[0m'

USER_PREFERENCES = 'examples/datasets/verifying_pac/user_preferences.csv'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)


print(
    f'''This example illustrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs).
A Domain PAC on a column set X and domain D, with given ε and δ means that {BOLD}Pr(x ∈ D±ε) ≥ δ{ENDC}.
For more information, see "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Filp Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).
If you have not read the basic Domain PAC examples yet (see the {CYAN}examples/basic/verifying_pac/{ENDC}
directory), it is recommended to start there.

Assume we have a dataset of user preferences, where each user\'s interest in several topics is
encoded as values in [0, 1], where 0 is "not interested at all" and 1 is "very interested":
{BOLD}{csv_to_str(USER_PREFERENCES)}{ENDC}

We need to estimate whether this group of users will be interested in the original Domain PAC paper
("Checks and Balances: ...").
To do this, we represent each user profile as a vector in a multi-dimensional topic space:
     ^ Topic 2
     |
     |   user
     |  x
     | /
     |/    Topic 1
    -+------->
     |

A "perfect" target reader might have the profile: {BLUE}(0.9, 0.4, 0.05){ENDC}.
This corresponds to:
    * high interest in Databases;
    * moderate interest in Networks;
    * low interest in Machine Learning.
Our goal is to measure how close real users are to this ideal profile.

We use cosine distance, which measures the angle between two vectors rather then their absolute
length. This is useful because we care about interest proportions, not total magnitude.
    {BOLD}dist(x, y) = 1 - cos(angle between x and y) = 1 - (x, y)/(|x| * |y|){ENDC},
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


print(f'''A custom domain is defined by two parameters:
    1. Distance function -- takes a value tuple and returns the distance to the domain.
    2. Domain name (optional) -- used for readable output.
In this example:
    * distance function: {BLUE}dist(x, (0.9, 0.4, 0.05)){ENDC};
    * domain name: {BLUE}"(0.9, 0.4, 0.05)"{ENDC}.
This effectively defines the domain as "users close to the ideal profile".
''')

PERFECT_USER = [0.9, 0.4, 0.05]


# Argument is always a list of strings
def dist_from_domain(x: list[str]) -> float:
    x_f = [float(x_i) for x_i in x]
    return cosine_dist(x_f, PERFECT_USER)


domain = desbordante.pac.domains.CustomDomain(dist_from_domain,
                                              "(0.9, 0.4, 0.05)")

print(f'We run the Domain PAC verifier with domain={BLUE}{domain}{ENDC}.')
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(USER_PREFERENCES, ',', True),
               domain=domain,
               column_indices=[0, 1, 2])
algo.execute()
pac_1 = algo.get_pac()
print(f'''Algorithm result:
    {GREEN}{pac_1}{ENDC}
Now we lower the required probability threshold: min_delta={BLUE}0.6{ENDC}.''')

algo.execute(min_delta=0.6)
pac_2 = algo.get_pac()

print(f'''Algorithm result:
    {GREEN}{pac_2}{ENDC}
Interpretation:
    * With a larger ε ({BLUE}{pac_1.epsilon:.3f}{ENDC}), nearly all users show some level of interest
    * With a very small ε ({BLUE}{pac_2.epsilon:.3f}{ENDC}), only {BLUE}{pac_2.delta * 100.0:.0f}%{ENDC} of users closely match the ideal reader.

You can user highlights to identify which users are closer to or farther from the ideal profile.
For an introduction to highlights, see {CYAN}examples/basic/verifying_pac/verifying_domain_pac1.py{ENDC}.'''
      )

# C++ note: Custom domain is called "Untyped domain" in C++ code, because it erases type
# information, converting all values to strings. If you use C++ library, it's recommended to
# implement IDomain interface or derive from MetricBasedDomain (if your domain is based on
# coordinate-wise metrics). See Parallelepiped and Ball implementations as examples.
