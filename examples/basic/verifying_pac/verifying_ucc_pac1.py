import desbordante

from util import *

WILDFIRE_SENSORS = "examples/datasets/verifying_pac/wildfire_sensors.csv"

print(
    f"""This example illustrates the use of Unique Column Combination Probabilistic Approximate
Constraints (UCC PACs). Given a column set X, a UCC PAC with parameters ε and δ specifies it is
unlikely that more than one tuple exists with approximately the same key:
    {BOLD}Pr(dist(t₁[Aᵢ] - t₂[Aᵢ]) ≤ ε) ≤ δ for each Aᵢ ∈ X{ENDC}
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Flip Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).

Consider a wildfire monitoring system based on a number of temperature sensors. The following table
contains coordinates of sensors that recorded elevated temperatures over a given period of time:"""
)
print(f"{BOLD}{csv_to_str(WILDFIRE_SENSORS)}{ENDC}")

print(f"""
When a single sensor raises an alarm, it does not necessarily indicate a wildfire. A sensor may have
been heated by sunlight or a campfire, or there may have been polling or software errors. Instead,
we should focus on situations in which several sensors raise alarms within a small geographic area.
In addition, a small probability threshold should be considered, since nearby sensors may have been
affected by the same non-wildfire heat source.

A good initial approximation is a distance threshold of 1 meter and a probability threshold of 10%.
In UCC PAC terms, this means that the following UCC PAC should hold:
    {BOLD}Pr(|t₁[Latitude] - t₂[Longitude]| ≤ 1) ≤ 0.1{ENDC}

Let's run UCC PAC Verifier with the following parameters: column_indices={BLUE}[0, 1]{ENDC}.""")

algo = desbordante.pac_verification.algorithms.UCCPACVerifier()
algo.load_data(table=(WILDFIRE_SENSORS, ",", True), column_indices=[0, 1])
algo.execute()

pac = algo.get_pac()
print(f"Algorithm result: {GREEN}{pac}{ENDC}.")

print(f"""
This PAC indicates that only {pac.delta * 100:.2f}% of the alarmed sensors were relatively close to
(distance no more than {pac.epsilon:.0f}). Therefore, it is unlikely that there is a wildfire.

We can gain additional insight by inspecting outliers (also called highlights) -- pairs of tuples
for which the PAC does not hold for a given ε.

To determine which alarmed sensors are located particularly close to one another, let's examine the
outliers for ε ∈ {BLUE}(0, {pac.epsilon:.0f}]{ENDC}.
""")

# get_highlights takes two arguments: eps_1 and eps_2 and returns highlights in (eps_1, eps_2]
# The default values are 0 and pac.epsilon
highlights = algo.get_highlights()
print(f"Outliers in (0, {pac.epsilon:.0f}]:")
for tp in highlights.string_data:
    print(f"\t{tp[0]}  {tp[1]}")

print(f"""
These are pairs of tuples whose distances fall within the interval {BLUE}(0, {pac.epsilon:.0f}){ENDC}.
You can find additional examples of outlier analysis in the Domain PAC examples. Note that, unlike
Domain PACs, UCC PAC outliers are pairs of tuples rather than individual tuples. In practice,
clustering these pairs before further processing may be useful.

Now that you are familiar with the basics of UCC PACs, you can continue with the second UCC PAC
example: {CYAN}examples/basic/verifying_pac/verifying_domain_pac2.py{ENDC}. This example demonstrates
the insights that can be gained by examining how δ depends on ε.""")
