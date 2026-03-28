import desbordante

from util import *

MARINE_URCHINS = "examples/datasets/verifying_pac/marine_urchins.csv"

print(
    f"""This example illustrates the usage of Functional Dependency Probabilistic Approximate Constraints (FD PACs).
Given columns sets X and Y, and a set of numbers {{Δᵢ}}, an FD PAC X → Y with parameters {{εᵢ}}, {{δᵢ}} means that
    {BOLD}if, for some tuples t₁, t₂, d(t₁[Aᵢ], t₂[Aᵢ]) < Δᵢ for each Aᵢ ∈ X, then
    Pr(d(t₁[Bᵢ], t₂[Bᵢ]) ≤ εᵢ) ≥ δᵢ for each Bᵢ ∈ Y{ENDC}
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Flip Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).

{CYAN}In this example, we use FD PACs with a single ε, i. e. ε₁ = ε₂ = ... = ε, since the FD PAC Verifier
supports only this setting.{ENDC}

Consider a study of a population of marine urchins in a coastal bay.
For each individual, we measured:
 - Age (in days)
 - Size (diameter in millimeters)
 - Water quality in the habitat, labeled from A (excellent) to D (poor)
The collected data is shown below:"""
)
print(f"{BOLD}{csv_to_str(MARINE_URCHINS)}{ENDC}")

print(f"""
We start with a natural hypothesis:
    {BOLD}The size of a marine urchin roughly depends on its age{ENDC}
In terms of FD PAC, we expect the dependency {BLUE}{{Age}} → {{Size}}{ENDC} to hold with reasonable parameters.

Let's run FD PAC verifier with the following parameters: lhs_indices={BLUE}[0]{ENDC}, rhs_indices={BLUE}[1]{ENDC},
lhs_deltas={BLUE}[10]{ENDC}.
""")

algo = desbordante.pac_verification.algorithms.FDPACVerifier()
algo.load_data(table=(MARINE_URCHINS, ",", True), lhs_indices=[0], rhs_indices=[1])
algo.execute(lhs_deltas=[10])

pac = algo.get_pac()
print(f"Algorithm result: {RED}{pac}{ENDC}.")

print(f"""
At first glance, the probability δ is high. However, the required ε = {pac.epsilons[0]} is quite large -- it
covers nearly half of the observed size range.
This indicates that age alone is not sufficient to explain the variability in size.

A natural explanation is environmental:
    {BOLD}Urchins living in poorer water conditions tend to be smaller than those in cleaner water.{ENDC}
This suggests refining the dependency: {BLUE}{{Age, Water quality}} → {{Size}}{ENDC}.

To validate this, we include {BLUE}Water quality{ENDC} in the left-hand side. Since the column is categorical,
we define a custom metric:
    {CYAN}A ~ 1, B ~ 2, C ~ 3, D ~ 4{ENDC}
This way, we respect the ordering of water quality levels instead of relying on the default
Levenshtein distance.
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

pac = algo.get_pac()
print(f"Algorithm result: {GREEN}{pac}{ENDC}.")

print(f"""
Now both parameters are meaningful:
 - ε = {pac.epsilons[0]}: reasonable size tolerance
 - δ = {pac.delta:.3f}: strong statistical support
This confirms that the refined hypothesis holds.""")
