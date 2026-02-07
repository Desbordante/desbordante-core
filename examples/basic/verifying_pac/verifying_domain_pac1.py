'''Example 1: 1D segment, highlights'''

from tabulate import tabulate
from csv import reader

import desbordante

RED = '\033[31m'
YELLOW = '\033[33m'
BOLD_YELLOW = '\033[1;33m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
BOLD = '\033[1;37m'
ENDC = '\033[0m'

ENGINE_TEMPS_BAD = 'examples/datasets/verifying_pac/engine_temps_bad.csv'
ENGINE_TEMPS_GOOD = 'examples/datasets/verifying_pac/engine_temps_good.csv'


def read_column(filename: str, col_num: int) -> (str, list[str]):
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    header = rows[0][col_num]
    values = [row[col_num] for row in rows[1:]]
    return header, values


def column_to_str(filename: str, col_num: int) -> str:
    header, values = read_column(filename, col_num)
    values_str = ', '.join(values)
    return f'{BOLD}{header}: [{values_str}]{ENDC}'


def display_columns_diff(filename_old: str, col_num_old: int,
                         filename_new: str, col_num_new: int) -> str:
    _, values_old = read_column(filename_old, col_num_old)
    header, values_new = read_column(filename_new, col_num_new)
    values = []
    for i in range(len(values_new)):
        value = values_new[i]
        if values_old[i] != value:
            value = f'{BOLD_YELLOW}' + value + f'{BOLD}'
        values.append(value)
    values_str = ', '.join(values)
    return f'{BOLD}{header}: [{values_str}]{ENDC}'


print(
    f'''This example illustrates the usage of Domain Probabilistic Approximate Constraints (PACs).
A Domain PAC on column set X and domain D, with given ε and δ means that Pr(x ∈ D±ε) ≥ δ.
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Flip Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).

This is the first example in the "Basic Domain PAC verification" series. Others can be found in
{CYAN}examples/basic/verifying_pac/{ENDC} directory.
''')

print(
    f'''Suppose we are working on a new model of engine. Its operating temperature range is {BLUE}[85, 95]{ENDC}°C.
The engine is made of high-strength metal, so short-term temperature deviations are acceptable and
will not cause immediate damage. In other words, engine operates properly when Pr(t ∈ [85, 95]±ε) ≥ δ.
Based on engineering analysis, the acceptable limits are: ε = {BLUE}5{ENDC}, δ = {BLUE}0.9{ENDC}.
In terms of Domain PACs, the following constraint should hold: {BLUE}Pr(x ∈ [85, 95]±5) ≥ 0.9{ENDC}.
''')

print(
    'The following table contains readings from the engine temperature sensor:'
)
# Values are printed in one line for brevity, original table is single-column
print(f'{column_to_str(ENGINE_TEMPS_BAD, 0)}')
print()

print(
    'We now use the Domain PAC verifier to determine whether the engine is operating safely.'
)

print(
    'First, we need to define the domain. A segment is a special case of a parallelepiped, so we use it here.'
)
# Parallelepiped has a special constructor for segment.
# Notice the usage of quotes: these strings will be converted to values once the table is loaded.
segment = desbordante.pac.domains.Parallelepiped('85', '95')

print(
    f'''We run algorithm with the following options: domain={BLUE}{segment}{ENDC}.
All other parameters use default values: min_epsilon={BLUE}0{ENDC}, max_epsilon={BLUE}∞{ENDC}, min_delta={BLUE}0.9{ENDC}, delta_steps={BLUE}100{ENDC}.
''')

algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
# Note that domain should be set in `load_data`, not `execute`
algo.load_data(table=(ENGINE_TEMPS_BAD, ',', True),
               column_indices=[0],
               domain=segment)
algo.execute()

print(f'Algorithm result: {YELLOW}{algo.get_pac()}{ENDC}.')
print(
    f'''This PAC is not very informative. Let\'s run algorithm with min_epsilon={BLUE}5{ENDC} and max_epsilon={BLUE}5{ENDC}.
This will give us the exact δ, for which PAC with ε={BLUE}5{ENDC} holds.
''')

# Note that, when min_epsilon or max_epsilon is specified, default min_delta becomes 0
algo.execute(min_epsilon=5, max_epsilon=5)

print(f'Algorithm result: {RED}{algo.get_pac()}{ENDC}.')
print(
    f'''Also, let\'s run algorithm with max_epsilon={BLUE}0{ENDC} and min_delta={BLUE}0.9{ENDC} to check which ε
is needed to satisfy δ={BLUE}0.9{ENDC}. With these parameters algorithm enters special mode and returns
pair (ε, min_delta), so that we can validate PAC with the given δ.
''')

# Actually, algorithm enters this mode whenever max_epsilon is less than epsilon needed to satisfy
# min_delta.
algo.execute(max_epsilon=0, min_delta=0.9)

pac = algo.get_pac()
print(f'Algorithm result: {RED}{pac}{ENDC}.')
print(
    f'''Here algorithm gives δ={BLUE}{pac.delta}{ENDC}, which is greater than {BLUE}0.9{ENDC}, because achieving δ={BLUE}0.9{ENDC} requires
ε={BLUE}{pac.epsilon}{ENDC} and PAC ({BLUE}{pac.epsilon}{ENDC}, {BLUE}{pac.delta}{ENDC}) holds. So, this means that δ={BLUE}0.9{ENDC} would also require ε={BLUE}{pac.epsilon}{ENDC}.
''')

print(
    'We can see that desired PAC doesn\'t hold, so the engine can blow up!\n')

print(
    f'''Let\'s look at values violating PAC. Domain PAC verifier can detect values between eps_1
and eps_2, i. e. values that lie in D±eps_2 \\ D±eps_1. Such values are called highlights or outliers.
Let\'s find outliers for different eps_1, eps_2 values:''')

value_ranges = [(0, 1), (1, 2), (2, 3), (3, 5), (5, 7), (7, 10)]
highlights_table = [(f'{BLUE}{v_range[0]}{ENDC}', f'{BLUE}{v_range[1]}{ENDC}',
                     str(algo.get_highlights(*v_range)))
                    for v_range in value_ranges]
print(tabulate(highlights_table, headers=('eps_1', 'eps_2', 'highlights')))
print()

print('''We can see two problems:
    1. The engine operated at low temperatures for an extended period, slightly below 80°C.
    2. The peak temperature was too high, but this occured only once.\n''')

print('''The second version of engine has:
    1. A pre-heating system to prevent operation at low temperatures.
    2. An emergency cooling system to limit peak temperatures.
The updated sensor readings (modified values highlighted) are:''')
print(f'{display_columns_diff(ENGINE_TEMPS_BAD, 0, ENGINE_TEMPS_GOOD, 0)}')
print()

print(f'''We run the Domain PAC verifier again.''')
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS_GOOD, ',', True),
               column_indices=[0],
               domain=segment)
algo.execute()

print(f'''Algorithm result: {GREEN}{algo.get_pac()}{ENDC}.
The desired PAC now holds, which means the improved engine operates within acceptable limits.

It is recommended to continue with the second example ({CYAN}examples/basic/verifying_pac/verifying_domain_pac2.py{ENDC}),
which demonstrates more advanced usage of the Parallelepiped domain.''')
