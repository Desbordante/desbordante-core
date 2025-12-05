'''Example 1: 1D segment, highlights'''

from tabulate import tabulate
from csv import reader

import desbordante

RED = '\033[31m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
GRAY = '\033[1;30m'
ENDC = '\033[0m'

ENGINE_TEMPS_BAD = 'examples/datasets/verifying_pac/engine_temps_bad.csv'
ENGINE_TEMPS_GOOD = 'examples/datasets/verifying_pac/engine_temps_good.csv'


def column_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    values = ', '.join(list(map(lambda row: str(row[0]), rows)))
    return f'{headers[0]}: [{values}]'


print(
    f'''{CYAN}This example illustrates the usage of Domain Probabilistic Approximate Constraints (PACs).
Domain PAC on column set X and domain D, with given epsilon and delta means that Pr(x ∈ D±epsilon) ≥ delta.
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Flip Korn et al.

This is the first example in "Basic Domain PAC verification" series. Others can be found in
examples/basic/verifying_pac/{ENDC} directory.
''')

print(
    f'''Consider we are working on a new model of engine. It\'s working temperatures lie in span {BLUE}[85, 95]{ENDC}°C.
Engine is made of high-strenght metal, so slight short-term temperature deviations won\'t kill it.
In other words, engine works properly when Pr(t ∈ [85, 95]±epsilon) ≥ delta.
Our engieneers have figured out limits: epsilon = {BLUE}5{ENDC}, delta = {BLUE}0.9{ENDC}.
So, in terms of Domain PACs, {BLUE}Domain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9{ENDC} should hold.
''')

print('The following table contains readings of engine temperature sensor:')
# Values are printed in one line for brevity, original table is single-column
print(f'{GRAY}{column_to_str(ENGINE_TEMPS_BAD)}{ENDC}')
print()

print('Let\'s use Domain PAC verifier to check if engine will be damaged\n')

print(
    'Firstly we need to create domain. Segment is a special case of parallelepiped, so let\'s use it.'
)
# Parallelepiped has a special constructor for segment.
# Notice the usage of quotes: these strings will be converted to values once table is loaded.
segment = desbordante.pac.domains.Parallelepiped('85', '95')

# TODO(senichenkov): diagonal_threshold example
print(
    f'''Now let\'s run algorithm with the following options: domain={BLUE}{segment}{ENDC}, max_epsilon={BLUE}10{ENDC}, min_delta={BLUE}0.85{ENDC}.
Max_epsilon should be greater than desired epsilon, min_delta -- a little less than the expected one.
We will use default values of other options: min_epsilon={BLUE}0{ENDC}, epsilon_steps={BLUE}100{ENDC}, diagonal_threshold={BLUE}1e-5{ENDC}.
Min_epsilon, max_epsilon and epsilon_steps contol which epsilon values and how many of them will be checked by the algorithm.
Diagonal threshold is advanced parameter, that is explained in {CYAN}examples/advanced/verifying_pac/##EXAMPLE_NAME##{ENDC}.
''')
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
# Note that domain should be set in `load_data`, not `execute`
algo.load_data(table=(ENGINE_TEMPS_BAD, ',', True),
               column_indices=[0],
               domain=segment)
algo.execute(max_epsilon=10, min_delta=0.85)

print(f'Algorithm result: {RED}{algo.get_pac()}{ENDC}.')
print('Uh-oh! The desired PAC doesn\'t hold, so engine can blow up!\n')

print(
    f'''Let\'s look at values violating PAC. Domain PAC verifier can detect values between eps_1
and eps_2, i. e. values that lie in D±eps_2 \\ D±eps_1. Such values are called highlights.
Let\'s check highlights for different eps_1, eps_2 values:''')

value_ranges = [(0, 1), (1, 2), (2, 3), (3, 5), (5, 7), (7, 10)]
highlights_table = [(f'{BLUE}{v_range[0]}{ENDC}', f'{BLUE}{v_range[1]}{ENDC}',
                     str(algo.get_highlights(*v_range)))
                    for v_range in value_ranges]
print(tabulate(highlights_table, headers=('eps_1', 'eps_2', 'highlights')))
print()

print('''We can see two problems:
      1. Time engine worked on low temperature was too long, but these temperatures were just a little lower than 80°C.
      2. Peak temperature was too high, but it has been reached only once.\n'''
      )

print('''Second version of engine has:
      1. pre-heating system to prevent engine from working on low temperatures;
      2. emergency cooling system to lower peak temperatures.
Let\'s look at second version\'s sensor readings:''')
print(f'{GRAY}{column_to_str(ENGINE_TEMPS_GOOD)}{ENDC}')
print()

print(
    f'''Let\'s run Domain PAC verifier with same parameters (domain={BLUE}{segment}{ENDC}, max_epsilon={BLUE}10{ENDC}).'''
)
algo = desbordante.pac_verification.algorithms.DomainPACVerifier()
algo.load_data(table=(ENGINE_TEMPS_GOOD, ',', True),
               column_indices=[0],
               domain=segment)
algo.execute(max_epsilon=10, min_delta=0.85)

print(f'Algorithm result: {RED}{algo.get_pac()}{ENDC}.')
print(
    f'''This PAC says that epsilon={BLUE}6{ENDC} is enough to cover all possible values. It\'s true, but this information is useless.
Let\'s select max_epsilon={BLUE}5.5{ENDC} and min_delta={BLUE}0.87{ENDC} to give algorithm a hint which values do we want.'''
)
# Max_epsilon is an "execute option", so `load_data` is not needed
algo.execute(max_epsilon=5.5, min_delta=0.87)

print(f'''Algorithm result: {GREEN}{algo.get_pac()}{ENDC}.
Our desired PAC holds, which means that engine works well.

It's recommended to continue with reading second example ({CYAN}examples/basic/verifying_pac/verifying_domain_pac2.py{ENDC}),
which demonstrates more advanced usage of the Parallelepiped domain.''')
