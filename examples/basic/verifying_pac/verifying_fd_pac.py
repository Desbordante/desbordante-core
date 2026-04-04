import logging
from sys import stdout

logging.basicConfig(stream=stdout)
logging.getLogger('desbordante').setLevel(logging.DEBUG)

import desbordante

from util import *

MARINE_URCHINS = "examples/datasets/verifying_pac/marine_urchins.csv"

print(
    """This example illustrates the usage of Functional Dependency Probabilistic Approximate Constraints (FD PACs).
Given columns sets X and Y, and a set of numbers {Δᵢ}, an FD PAC X → Y with parameters {εᵢ}, {δᵢ} means that
    if, for some tuples t₁, t₂, d(t₁[Aᵢ], t₂[Aᵢ]) < Δᵢ for each Aᵢ ∈ X, then
    Pr(d(t₁[Bᵢ], t₂[Bᵢ]) ≤ εᵢ) ≥ δᵢ for each Bᵢ ∈ Y
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Flip Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).

Note that all FD PACs in this example have ε₁ = ε₂ = ... = ε, because FD PAC Verifier can verify
only these FD PACs.

Consider we are researching a population of marine urchins in a coastal bay.
For each individual, we measured:
 - Age (in days)
 - Size (diameter in millimeters)
 - Water quality in individual's habitat, labeled from A (excellent) to D (poor)
The following table contains measurement results:"""
)
print(f"{BOLD}{csv_to_str(MARINE_URCHINS)}{ENDC}")

print(f"""
We can assume that the size of marine urchin roughly depends on its age.
In terms of FD PACs, this means that FD PAC {BLUE}{{Age}} → {{Size}}{ENDC} should hold with some reasonable parameters.

Let's run FD PAC verifier to check this hypothesis. We use the following parameters: lhs_indices={BLUE}[0]{ENDC},
rhs_indices={BLUE}[1]{ENDC}, lhs_deltas={BLUE}[10]{ENDC}.
""")

algo = desbordante.pac_verification.algorithms.FDPACVerifier()
algo.load_data(table=(MARINE_URCHINS, ",", True), lhs_indices=[0], rhs_indices=[1])
algo.execute(lhs_deltas=[10])

pac = algo.get_pac()
print(f"Algorithm result: {RED}{pac}{ENDC}.")

print(f"""
This FD PAC means that our hypothesis holds only for {pac.deltas[0] / 100:.2f}% of urchins.

Intuitively, we can suggest, that urchins which live in poor water, should be smaller, than those
which live in clear water.
In terms of FD PACs, this means that we need to validate FD PAC {BLUE}{{Age, Water quality}} → {{Size}}{ENDC}.

Let's run the algorithm with the following parameters: lhs_indices={BLUE}[0, 2]{ENDC}, rhs_indices={BLUE}[1]{ENDC},
lhs_deltas={BLUE}[10]{ENDC}.

{RED}TODO: сказать про метрику{ENDC}
""")


# Note that all custom metrics must work with strings.
# If you need type info, consider using FD PAC verifier from C++ library
def alphabet_dist(a: str, b: str) -> float:
    return abs(ord(a[0]) - ord(b[0]))


algo = desbordante.pac_verification.algorithms.FDPACVerifier()
algo.load_data(
    table=(MARINE_URCHINS, ",", True),
    lhs_indices=[0, 2],
    rhs_indices=[1],
    # None means "default metric for this column"
    lhs_metrics=[None, alphabet_dist],
)
algo.execute(lhs_deltas=[10, 1])

alphabet_dist('abc', 'cba')

pac = algo.get_pac()
print(f"Algorithm result: {GREEN}{pac}{ENDC}.")

print("""This PAC means that our refined hypothesis holds.""")
