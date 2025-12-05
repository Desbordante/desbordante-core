'''Example 4: string values'''

from tabulate import tabulate
from csv import reader

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
GRAY = '\033[1;30m'
ENDC = '\033[0m'

LEVENSHTEIN_TYPOS = 'examples/datasets/verifying_pac/levenshtein_typos.csv'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)


print(
    f'''{CYAN}This example illustrates the usage of Domain Probabilistic Approximate Constraints (PACs).
This example is the last of "Basic Domain PAC verification" series (see examples/basic/verifying_pac/ directory).
If you haven\'t read first three parts, start with reading them.{ENDC}

Consider the following dataset of users\' attempts to type a difficult spanish word:
{GRAY}{csv_to_str(LEVENSHTEIN_TYPOS)}{ENDC}

We want to prove that most of our users can remember difficult words almost exactly.
In other words, we want to verify {BLUE}Domain PAC Pr(dist(x, "Desbordante") ≤ 3) ≥ 0.9{ENDC}.

As a metric between words we will use Levenshtein distance that shows how much characters must be replaced,
deleted or inserted into first string to get the second one. We are lucky that this is the default
metric for strings in Desbordante. If you read previous examples, you may know that the most suitable
domain in this case is Ball.

Note that, regardless of domain choice, metric must agree with comparer, i. e.
x ∈ D iff ∃ a, b : a ≤ x ≤ b (otherwise, behaviour is undefined).
The default comparer for strings is lexicographical compare, which agrees only with "lexicographical
metric": distance from a to b is a number of words between a and b in lexicographically-sorted dictionary.
This means that domain, that uses default comparer, cannot be used with string values.
Fortunately, Ball uses "radius" comparer: a < b iff dist(a, center) < dist(b, center).
Therefore, if table contains string values, you have to use either Ball or Custom domain
(see {GRAY}examples/advanced/verifying_pac/verifying_domain_pac_custom_domain.py{ENDC}).
''')

ball = desbordante.pac.domains.Ball(["Desbordante"], 1)

print(
    f'Let\'s run Domain PAC verifier with the following options: domain={BLUE}{ball}{ENDC}, max_epsilon={BLUE}3{ENDC}.'
)
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(LEVENSHTEIN_TYPOS, ',', True),
               domain=ball,
               column_indices=[0])
algo.execute(max_epsilon=3)
pac = algo.get_pac()
print(f'Result: {GREEN}{pac}{ENDC}.')

print(
    f'''This means that {GREEN}{pac.delta * 100}%{ENDC} of our users make no more than {GREEN}{pac.epsilon}{ENDC} typos in word "Desbordante", which is good enough.
''')

print(
    f'''Now that you\'ve read all basic examples, you can check {CYAN}examples/advanced/verifying_pac/verifying_domain_pac_custom_domain.py{ENDC},
which demonstrates the usage of Custom domain.''')
