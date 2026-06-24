import textwrap

import desbordante
import pandas as pd
from tabulate import tabulate

CYAN = "\033[1;36m"
YELLOW = "\033[1;33m"
RESET = "\033[0m"

TABLE_PATH = "examples/datasets/ticket_routing.csv"
PREFERRED_RULES = [
    (["Contract", "Region"], "Queue"),
    (["Contract", "Queue"], "Segment"),
    (["Segment"], "SLA"),
]
VIOLATION_EXPLANATIONS = {
    9: "Likely typo: expected Queue-A, got Queu-A (missing letter 'e').",
    10: "Different routing direction: for US expected Queue-B, but got Queue-A. "
        "This can be a typo or an upstream routing error.",
}
SUPPORT_THRESHOLDS = [1, 2, 3, 4]
VALIDATION_RULES = [
    ([("Region", "EU")], ("Queue", "Queue-A")),
    ([("Region", "US")], ("Queue", "Queue-B")),
]

def printlns(s):
    print(textwrap.fill(s, 80))
    print()


def banner(title):
    print("=" * 80)
    print(f"{CYAN}{title}{RESET}")
    print("=" * 80)


def print_table(df, title):
    print(title)
    print(tabulate(df, headers="keys", tablefmt="psql", showindex=False))
    print()

def mine_cfun(df, min_support):
    algo = desbordante.cfd.algorithms.CFUN()
    algo.load_data(table=df)
    algo.execute(cfd_minsup=min_support)
    return algo.get_cfds()

def average_support_per_cfd(cfds):
    if not cfds:
        return 0.0
    return sum(cfd.support for cfd in cfds) / len(cfds)


def verify_cfd_rule(df, lhs, rhs, min_conf=1.0):
    verifier = desbordante.cfd_verification.algorithms.Default()
    verifier.load_data(table=df)
    verifier.execute(cfd_rule_left=lhs, cfd_rule_right=rhs, cfd_minconf=min_conf)
    return verifier


def collect_violating_rows(verifier):
    violating_rows = set()
    for highlight in verifier.get_highlights():
        violating_rows.update(highlight.get_violating_rows)
    return violating_rows


def find_cfd(cfds, lhs_names, rhs_name):
    lhs_tuple = tuple(lhs_names)
    for cfd in cfds:
        lhs, rhs = cfd.embedded_fd.to_name_tuple()
        if tuple(lhs) == lhs_tuple and rhs == rhs_name:
            return cfd
    return None


def tableau_to_dataframe(cfd):
    lhs, rhs = cfd.embedded_fd.to_name_tuple()
    pattern_columns = list(lhs) + [rhs]
    rows = []

    for condition in cfd.tableau:
        pattern = list(condition.pattern)
        row = {}

        for idx, col in enumerate(pattern_columns):
            row[col] = pattern[idx] if idx < len(pattern) else None

        for extra_idx, value in enumerate(pattern[len(pattern_columns):], start=1):
            row[f"pattern_extra_{extra_idx}"] = value

        row["support"] = condition.support
        rows.append(row)

    if not rows:
        return pd.DataFrame(columns=pattern_columns + ["support"])
    return pd.DataFrame(rows)


def explain_cfd(cfd):
    lhs, rhs = cfd.embedded_fd.to_name_tuple()
    key = (tuple(lhs), rhs)
    explanations = {
        (("Contract", "Region"), "Queue"): (
            "This CFD reflects a stable routing policy: once contract type and region are fixed, "
            "the ticket should consistently go to a single operational queue. "
            "In the mined patterns here, Monthly+EU routes to Queue-A and Monthly+US routes to Queue-B. "
            "In practice, this rule helps detect misrouted requests and drift in triage logic."
        ),
        (("Segment", "Region"), "Queue"): (
            "This CFD captures segmentation-aware routing behavior: within each market region, "
            "customer segment determines the expected handling queue. "
            "In the mined patterns here, Enterprise+EU maps to Queue-A and Enterprise+US maps to Queue-B. "
            "Operationally, it is useful for checking whether enterprise and retail flows are separated as intended."
        ),
        (("Segment",), "SLA"): (
            "This CFD expresses a service-policy pattern: customer segment is associated with a "
            "consistent SLA tier. In the mined patterns here, Enterprise maps to Premium. "
            "It helps verify whether prioritization rules are applied uniformly and can highlight records "
            "where support level assignment looks suspicious."
        ),
        (("Contract", "Queue"), "Segment"): (
            "This CFD shows a conditional customer-mix pattern: for specific contract and queue "
            "combinations, the segment is fixed. In the mined patterns here, Annual+Queue-B maps to "
            "Enterprise. This can help detect when requests arrive in a queue with an unexpected segment."
        ),
    }
    return explanations.get(
        key,
        f"This CFD means that values of [{', '.join(lhs)}] determine a single value of {rhs}.",
    )


def print_intro():
    banner("Discovering Frequent Constant CFDs with CFUN")

    printlns(
        "This example demonstrates how to discover constant conditional functional "
        "dependencies in data using the CFUN algorithm from the paper T. Diallo, N."
        "Novelli, J.-M. Petit. Discovering (frequent) constant conditional functional"
        "dependencies. Int. J. Data Mining, Modelling and Management, 2010. "
        "It also shows canonical-cover intuition "
        "and a simple data-quality application."
    )
    print(f"{YELLOW}>>> Definition (FD).{RESET}")
    printlns(
        "A functional dependency (FD) X -> A is defined on table attributes, "
        "where X is a set of attribute(s) and A is an attribute. It means: if "
        "two rows agree on X, they must agree on A."
    )
    print("Example: Disease -> Department")
    printlns(
        "Meaning: if two rows have the same Disease, they must have the same "
        "Department."
    )

    print(f"{YELLOW}>>> Definition (constant CFD).{RESET}")
    printlns(
        "A constant CFD is a FD rule X -> A and a pattern tableau (table of "
        "patterns). Each pattern row contains values of the attributes from X "
        "and A. This tableau defines where the FD is applied: among rows "
        "matching a pattern on X, the value of A is fixed."
    )
    print("Example: [Disease] -> Department with patterns")
    print("  .Flu.| (Therapy)")
    print("  .Fracture.| (Orthopedics)")
    printlns(
        "Meaning: for rows with Disease=Flu, Department should be Therapy; "
        "for rows with Disease=Fracture, Department should be Orthopedics."
    )

    print(f"{YELLOW}>>> CFD metrics: support (or frequency).{RESET}")
    printlns(
        "For a constant CFD, support is the number of tuples that match all "
        "constants in its pattern. CFUN returns only dependencies whose patterns satisfy "
        "the support threshold cfd_minsup."
    )

    print(f"{YELLOW}>>> Canonical cover.{RESET}")
    printlns(
        "A canonical cover is a compact equivalent set of dependencies: rules "
        "are non-redundant and left-reduced, but together they preserve the "
        "same implications on the data. In practice this means we expect "
        "minimal rules and no obvious supersets with the same meaning. "
        "CFUN targets this frequent constant CFD cover, i.e., the most general and frequent rules in the data."
    )
    print("Simple example: if the algorithm finds [Contract Region] -> Queue,")
    print("it marks [Segment Contract Region] -> Queue as redundant and drops it.")
    print()


def print_dataset_description(df):
    print(f"{YELLOW}>>> Dataset description.{RESET}")
    printlns(
        "Consider synthetic queue-routing log with 12 tickets. "
        "Each row is one support request. Segment is customer type "
        "(Retail/Enterprise), Contract is billing cycle (Monthly/Annual), "
        "Region is geography (EU/US), SLA is service tier "
        "(Standard/Premium), Queue is the team that handled the ticket "
        "(Queue-A/Queue-B)."
    )
    print_table(df, "Training dataset:")


def run_mining_scenario(cfds_2):
    banner("Scenario 1. Mining a canonical cover with low support threshold")
    printlns(
        f"At a relatively low support threshold (cfd_minsup = 2), the algorithm finds {len(cfds_2)} CFDs. "
        "Below are a few examples:"
    )

    shown = []
    for lhs, rhs in PREFERRED_RULES:
        cfd = find_cfd(cfds_2, lhs, rhs)
        if cfd is not None:
            shown.append(cfd)

    for idx, cfd in enumerate(shown, start=1):
        pattern_df = tableau_to_dataframe(cfd)

        print(f"CFD #{idx}: {cfd.embedded_fd}")
        print_table(pattern_df, "Pattern tableau:")
        printlns(f"{explain_cfd(cfd)}")
        print()

def run_support_impact_scenario(cfds_by_threshold):
    banner("Scenario 2. Increasing support threshold")
    support_impact = pd.DataFrame(
        [
            {
                "cfd_minsup": threshold,
                "discovered_cfds": len(cfds_by_threshold[threshold]),
                "avg_support": round(average_support_per_cfd(cfds_by_threshold[threshold]), 2),
            }
            for threshold in SUPPORT_THRESHOLDS
        ]
    )
    printlns(
        "The number of discovered dependencies strongly depends on cfd_minsup. "
        "With a smaller support threshold, the algorithm tends to find more "
        "weak local rules. As the threshold increases, fewer CFDs pass support, "
        "so the output becomes smaller and keeps mostly stronger, more frequent patterns."
    )
    print_table(support_impact, "Support threshold impact:")


def run_usecase(df, cfds_2):
    banner("Scenario 3. Missing expected business CFD and typo discovery")
    printlns(
        "Assume we expect the following business rule to hold in your dataset: routing should "
        "be region-specific, i.e., EU tickets should go to Queue-A and US tickets should go "
        "to Queue-B. This rule can be expressed as the CFD rule"
    )
    
    expected_rule_df = pd.DataFrame(
        [
            {"Region": "EU", "Expected_Queue": "Queue-A"},
            {"Region": "US", "Expected_Queue": "Queue-B"},
        ]
    )
    printlns("CFD: [Region] -> Queue")
    print_table(expected_rule_df, "Pattern tableau:")

    printlns("We mine CFDs with cfd_minsup=2 and check whether this rule appears in the discovered set.")
    
    expected_cfd = find_cfd(cfds_2, ["Region"], "Queue")
    if expected_cfd is None:
        print("Mined CFDs at cfd_minsup=2 do not contain [Region] -> Queue.\n")
    else:
        print("Mined CFDs at cfd_minsup=2 contain [Region] -> Queue.\n")

    printlns(
        "To detect violating rows, we validate concrete CFD rules with the cfd_verification module."
    )

    verification_summary = []
    violating_rows = set()
    for lhs_rule, rhs_rule in VALIDATION_RULES:
        verifier = verify_cfd_rule(df, lhs_rule, rhs_rule, min_conf=1.0)
        rows = collect_violating_rows(verifier)
        violating_rows.update(rows)
        verification_summary.append(
            {
                "rule": f"{lhs_rule} -> {rhs_rule}",
                "holds": verifier.cfd_holds(),
                "support": verifier.get_real_support(),
                "violating_rows": len(rows),
            }
        )

    print_table(pd.DataFrame(verification_summary), "CFD verification results:")

    if not violating_rows:
        print("No violating rows found by cfd_verification.")
    else:
        preview = df.loc[sorted(violating_rows), ["Region", "Queue"]].copy()
        preview.insert(0, "Row", preview.index + 1)
        print_table(preview, "Rows violating expected CFD:")
        printlns("Explanation: ")
        for row_num in preview["Row"].tolist():
            explanation = VIOLATION_EXPLANATIONS.get(
                row_num, "Violation detected by cfd_verification."
            )
            printlns(f"Row {row_num}: {explanation}")
        print()


def print_see_also():
    banner("See also")
    print("  * examples/basic/mining_cfd.py")
    print("  * examples/basic/verifying_cfd.py")
    print("  * examples/basic/mining_cind.py")
    print("  * examples/basic/verifying_cind.py")


def main():
    print_intro()
    df = pd.read_csv(TABLE_PATH)
    print_dataset_description(df)

    cfds_by_threshold = {threshold: mine_cfun(df, min_support=threshold)
                         for threshold in SUPPORT_THRESHOLDS}
    cfds_2 = cfds_by_threshold[2]

    run_mining_scenario(cfds_2)
    run_support_impact_scenario(cfds_by_threshold)
    run_usecase(df, cfds_2)
    print_see_also()


if __name__ == "__main__":
    main()
