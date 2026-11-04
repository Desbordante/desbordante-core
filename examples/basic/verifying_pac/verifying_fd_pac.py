import desbordante

from tabulate import tabulate

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
algo.load_data(
    table=(MARINE_URCHINS, ",", True), lhs_indices=[0], rhs_indices=[1], lhs_deltas=[10]
)
algo.execute()

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


def alphabet_dist(a: str, b: str) -> float:
    return abs(ord(a[0]) - ord(b[0]))


algo = desbordante.pac_verification.algorithms.FDPACVerifier()
algo.load_data(
    table=(MARINE_URCHINS, ",", True),
    lhs_indices=[0, 2],
    rhs_indices=[1],
    # None means "default metric for this column"
    lhs_metrics=[None, alphabet_dist],
    lhs_deltas=[10, 1],
)
algo.execute()

pac = algo.get_pac()
print(f"Algorithm result: {GREEN}{pac}{ENDC}.")

print(f"""
Now both parameters are meaningful:
 - ε = {pac.epsilons[0]}: reasonable size tolerance
 - δ = {pac.delta:.3f}: strong statistical support
This confirms that the refined hypothesis holds.""")

print(f"""
To better understand, why the PAC holds (or does not hold), we can examine outliers (also called
highlights) -- pairs of tuples for which the PAC does not hold for a given ε.

Let's inspect the outliers between ε={BLUE}9{ENDC} and ε={BLUE}10{ENDC}.""")

EPS_1 = 9
EPS_2 = 10
highlights = algo.get_highlights(EPS_1, EPS_2)
print(f"Outliers in ({EPS_1}, {EPS_2}]:")
for tp1, tp2 in highlights.string_data:

    def tp_to_str(tp: list[str]) -> str:
        return f"{tp[0]} -> {tp[1]}"

    print(f"\t{tp_to_str(tp1)},  {tp_to_str(tp2)}")
print(f"""
These pairs have distances in the right-hand side between {BLUE}9{ENDC} and {BLUE}10{ENDC}, which means that if
we removed these pairs from the table, the PAC with ε={BLUE}9{ENDC} and the same δ={BLUE}{pac.delta:.3f}{ENDC} would hold.

You can find more interesting usages of outliers in Domain PAC examples. Note that, unlike
Domain PAC, FD PAC outliers are not individual tuples, but pairs of tuples. It can be useful to
cluster them before processing.""")
