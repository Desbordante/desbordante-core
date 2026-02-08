'''Example 4: string values'''

from tabulate import tabulate
from csv import reader

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
BOLD = '\033[1;37m'
ENDC = '\033[0m'

LEVENSHTEIN_TYPOS = 'examples/datasets/verifying_pac/levenshtein_typos.csv'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)


print(
    f'''This example illustrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs).
It is the final example in the "Basic Domain PAC verification" series (see the {CYAN}examples/basic/verifying_pac/{ENDC} directory).
If you haven\'t read the first three parts yet, it is recommended to start there.

Consider the following dataset of users\' attempts to type a difficult Spanish word:
{BOLD}{csv_to_str(LEVENSHTEIN_TYPOS)}{ENDC}

We want to show that most users can remember difficult words almost exactly.
In probabilistic terms, we want to verify the following Domain PAC:
    {BLUE}Pr(dist(x, "Desbordante") ≤ 3) ≥ 0.9{ENDC}

To measure the similarity between words, we use the Levenshtein distance, which counts how many
character insertions, deletions, or substitutions are required to transform one string into another.
In Desbordante, Levenshtein distance is the default metric for strings, so no additional
configuration is needed.
Based on the previous examples, the most suitable domain here is the Ball domain, because we are
measuring distance from a single center value.
''')

ball = desbordante.pac.domains.Ball(["Desbordante"], 1)

print(
    f'We run the Domain PAC verifier with the following parameter: domain={BLUE}{ball}{ENDC}.'
)
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(LEVENSHTEIN_TYPOS, ',', True),
               domain=ball,
               column_indices=[0])
algo.execute()
pac = algo.get_pac()
print(f'Result: {GREEN}{pac}{ENDC}.')

print(
    f'''This means that {GREEN}{pac.delta * 100}%{ENDC} of our users make no more than {GREEN}{pac.epsilon}{ENDC} typos in the word
"Desbordante", which satisfies our requirement.
''')

print(
    f'''Now that you have completed all basic examples, you continue with the advanced example:
{CYAN}examples/advanced/verifying_pac/verifying_domain_pac_custom_domain.py{ENDC}.
This example demonstrates how to define and use a Custom domain.''')
