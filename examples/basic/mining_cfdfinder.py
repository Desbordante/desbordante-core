import textwrap

import desbordante
import pandas as pd
from tabulate import tabulate


CYAN = "\033[1;36m"
YELLOW = "\033[1;33m"
RESET = "\033[0m"

TABLE_PATH = "examples/datasets/bacteria.csv"
DEFINITIONS_TABLE_PATH = "examples/datasets/schedules.csv"

LEGACY_PARAMS = {
    "expansion_strategy": "negative_constant",
    "pruning_strategy": "legacy",
    "result_strategy": "direct",
    "cfd_minsup": 0.7,
    "cfd_minconf": 1.0,
    "max_lhs": 2,
}

SUPPORT_INDEPENDENT_PARAMS = {
    "expansion_strategy": "range",
    "pruning_strategy": "support_independent",
    "result_strategy": "direct",
    "cfd_minconf": 1.0,
    "min_support_gain": 4,
    "max_level_support_drop": 2,
    "max_patterns": 1,
    "rhs_indices": [4],  # Growth_Rate
    "max_lhs": 2,
}

PARTIAL_FD_PARAMS = {
    "expansion_strategy": "constant",
    "pruning_strategy": "partial_fd",
    "result_strategy": "direct",
    "max_g1": 0.08,
    "rhs_indices": [4],  # Growth_Rate
    "max_lhs": 3,
}


def printlns(text):
    print(textwrap.fill(text, 80))
    print()


def banner(title):
    print("=" * 80)
    print(f"{CYAN}{title}{RESET}")
    print("=" * 80)


def print_table(df, title):
    print(title)
    print(tabulate(df, headers="keys", tablefmt="psql", showindex=False))
    print()


def parameters_to_dataframe(parameters):
    return pd.DataFrame(parameters.items(), columns=["parameter", "value"])


def mine_cfds(df, **params):
    algo = desbordante.cfd.algorithms.CFDFinder()
    algo.load_data(table=df)
    algo.execute(**params)
    return algo.get_cfds()


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


def format_embedded_fd(embedded_fd):
    lhs, rhs = embedded_fd.to_name_tuple()
    return f"[{', '.join(lhs)}] -> {rhs}"


def tableau_to_dataframe(cfd):
    """Represent CFDFinder's LHS-only pattern tableau as a readable table."""
    lhs, rhs = cfd.embedded_fd.to_name_tuple()
    rows = []

    for row_number, pattern in enumerate(cfd.tableau, start=1):
        pattern = list(pattern)
        row = {"#": row_number}

        for index, column in enumerate(lhs):
            row[column] = pattern[index] if index < len(pattern) else None

        # CFDFinder always uses a wildcard condition on the RHS. The C++ API
        # therefore returns only the LHS part of every pattern tableau row.
        row[rhs] = "_"

        for extra_index, value in enumerate(pattern[len(lhs):], start=1):
            row[f"pattern_extra_{extra_index}"] = value

        rows.append(row)

    columns = ["#", *lhs, rhs]
    if not rows:
        return pd.DataFrame(columns=columns)
    return pd.DataFrame(rows)


def print_cfd(cfd, number=None):
    prefix = f"CFD #{number}: " if number is not None else "CFD: "
    print(f"{prefix}{format_embedded_fd(cfd.embedded_fd)}")
    print_table(tableau_to_dataframe(cfd), "Pattern tableau:")
    print(f"Support: {cfd.support:.4f}")
    print(f"Confidence: {cfd.confidence:.4f}")
    print()


def find_cfd(cfds, lhs_names, rhs_name):
    expected_lhs = tuple(lhs_names)
    for cfd in cfds:
        lhs, rhs = cfd.embedded_fd.to_name_tuple()
        if tuple(lhs) == expected_lhs and rhs == rhs_name:
            return cfd
    return None


def print_intro():
    banner("Discovering Conditional Functional Dependencies with CFDFinder")
    printlns(
        "This example demonstrates how to discover conditional functional "
        "dependencies (CFDs) with CFDFinder, the algorithm described in [1]. "
        "It also presents several types of conditions in CFDs and usage scenarios "
        "for the algorithm."
    )

    print(f"{CYAN}Note:{RESET}")
    printlns(
        "There are several definitions of CFDs, and different algorithms are designed for "
        "different definitions. Here we focus on CFDFinder. Links "
        "to examples for other CFD algorithms are listed at the end."
    )


def print_definitions():
    banner("Basic definitions")

    definition_df = pd.read_csv(DEFINITIONS_TABLE_PATH)
    definition_df.insert(0, "#", range(1, len(definition_df) + 1))
    print_table(definition_df, "Consider a small employee dataset:")

    print(f"{YELLOW}>>> Definition (FD).{RESET}")
    printlns(
        "A functional dependency (FD) X -> A is defined on table attributes, where "
        "X is a set of attributes and A is an attribute. It holds on a table if "
        "every two rows that agree on X also agree on A."
    )
    print("Example: [Department, Employment_Type] -> Work_Schedule")
    printlns(
        "This FD does not hold on the whole dataset. Rows #6 and #8 have the same "
        "Department=Support and Employment_Type=Full-time, but their Work_Schedule "
        "values differ."
    )

    print(f"{YELLOW}>>> Definition (CFD).{RESET}")
    printlns(
            "A conditional functional dependency (CFD) combines an embedded FD X -> A "
            "with a pattern tableau. For every attribute, a pattern-tableau row contains "
            "either a restriction on its value or the wildcard '_', which matches any "
            "value. A data row is covered by a pattern when it satisfies all restrictions "
            "in that pattern.")

    printlns("A CFD holds if, for each pattern, the embedded FD holds "
            "on the rows covered by that pattern.")

    cfd_tableau = pd.DataFrame(
        [
            {
                "#": 1,
                "Department": "IT",
                "Employment_Type": "_",
                "Work_Schedule": "_",
            },
            {
                "#": 2,
                "Department": "Support",
                "Employment_Type": "Part-time",
                "Work_Schedule": "_",
            },
        ]
    )
    print("Example:")
    print("Embedded FD: [Department, Employment_Type] -> Work_Schedule")
    print_table(cfd_tableau, "Pattern tableau:")
    printlns(
        "This CFD holds. Pattern #1 covers the IT employees in rows #1-#3, and "
        "pattern #2 covers the part-time Support employees in rows #4-#5. "
        "The full-time Support employees in rows #6 and #8 do not match pattern #2, "
        "and the Sales employee in row #7 matches neither pattern. "
        "Within the rows covered by the same pattern, every equal pair of Department and "
        "Employment_Type values has one Work_Schedule. Thus, the conflicting rows "
        "#6 and #8 are outside both contexts and do not violate the CFD. "
    )

    print(f"{CYAN}Note:{RESET}")
    printlns("CFDFinder discovers only CFDs whose patterns have '_' on the RHS.")

    print(f"{YELLOW}>>> Quality measures.{RESET}")
    printlns(
        "In real-world data, CFDs rarely hold "
        "exactly. Support and confidence describe how much data a CFD covers and "
        "how closely the covered rows follow its embedded FD."
    )

    metrics_tableau = pd.DataFrame(
        [
            {
                "#": 1,
                "Department": "IT",
                "Employment_Type": "_",
                "Work_Schedule": "_",
            },
            {
                "#": 2,
                "Department": "Support",
                "Employment_Type": "_",
                "Work_Schedule": "_",
            },
        ]
    )
    print(f"{CYAN}Example.{RESET}")
    print("Embedded FD: [Department, Employment_Type] -> Work_Schedule")
    print_table(metrics_tableau, "Pattern tableau:")
    printlns(
        "Pattern #1 covers the IT employees in rows #1-#3. Pattern #2 covers the "
        "Support employees in rows #4-#6 and #8."
    )

    print(f"{CYAN}1. Support.{RESET}")
    printlns(
        "Support is the fraction of all input rows covered by at least one pattern "
        "in the tableau. The example CFD covers seven of the eight input rows, so "
        "its support is 7/8=0.875."
    )

    print(f"{CYAN}2. Confidence.{RESET}")
    printlns(
        "Confidence is the fraction of covered rows that can be kept without "
        "violating the embedded FD. The IT and part-time Support rows satisfy the "
        "FD, but full-time Support rows #6 and #8 have equal LHS values and "
        "different Work_Schedule values. Removing either conflicting row makes the "
        "FD hold on the cover, so six of the seven covered rows are keepers and "
        "confidence is 6/7≈0.8571."
    )

    print(f"{YELLOW}>>> Definition (pattern bindings).{RESET}")
    printlns("Besides wildcards, CFDFinder supports three forms of LHS binding in patterns.")

    constant_tableau = pd.DataFrame(
        [
            {
                "#": 1,
                "Department": "IT",
                "Employment_Type": "_",
                "Work_Schedule": "_",
            },
        ]
    )
    print(f"{CYAN}1. Constant binding.{RESET}")
    print("Embedded FD: [Department, Employment_Type] -> Work_Schedule")
    print_table(constant_tableau, "Pattern tableau:")
    printlns(
        "The constant IT means Department=IT. Together with the wildcard, this "
        "pattern covers all IT employees, which are rows #1-#3."
    )

    negative_tableau = pd.DataFrame(
        [
            {
                "#": 1,
                "Department": "¬Support",
                "Employment_Type": "_",
                "Work_Schedule": "_",
            },
        ]
    )
    print(f"{CYAN}2. Negated constant binding.{RESET}")
    print("Embedded FD: [Department, Employment_Type] -> Work_Schedule")
    print_table(negative_tableau, "Pattern tableau:")
    printlns(
        "The condition ¬Support means Department!=Support. It covers rows #1-#3 "
        "and #7 and excludes the conflicting Support rows context. "
        "A negated constant can compactly describe a broad context by "
        "excluding one exceptional value."
    )

    range_tableau = pd.DataFrame(
        [
            {
                "#": 1,
                "Experience_Years": "[1 - 5]",
                "Work_Schedule": "_",
            },
        ]
    )
    print(f"{CYAN}3. Range binding.{RESET}")
    print("Embedded FD: [Experience_Years] -> Work_Schedule")
    print_table(range_tableau, "Pattern tableau:")
    printlns(
        "The inclusive range [1 - 5] covers rows #1-#5. Inside this range, every "
        "experience value is associated with one schedule. Rows #6-#8 are outside "
        "this range; in particular, Experience_Years=6 has conflicting schedules "
        "in rows #6 and #8."
    )


def print_parameters():
    banner("Algorithm parameters")
    printlns(
        "The algorithm has three main strategy parameters."
    )
    strategy_parameter_table = pd.DataFrame(
        [
            {
                "parameter": "expansion_strategy",
                "role": "controls the bindings allowed in patterns",
            },
            {
                "parameter": "pruning_strategy",
                "role": "determines the algorithm's usage scenario",
            },
            {
                "parameter": "result_strategy",
                "role": "filters the final set of discovered CFDs",
            },
        ]
    )
    print_table(strategy_parameter_table, "Main parameters (strategies):")


    print(f"{YELLOW}>>> expansion_strategy.{RESET}")
    expansion_strategy_table = pd.DataFrame(
        [
            {
                "value": "constant",
                "allowed bindings": "constants and the wildcard '_'",
            },
            {
                "value": "negative_constant",
                "allowed bindings": "constants, negated constants (¬value), and '_'",
            },
            {
                "value": "range",
                "allowed bindings": "ranges, constants, and '_'",
            },
        ]
    )
    print_table(expansion_strategy_table, "Expansion strategy variants:")
    printlns(
        "For range bindings, CFDFinder arranges the attribute domain in "
        "lexicographic order and uses intervals over that order."
    )

    print(f"{YELLOW}>>> pruning_strategy.{RESET}")
    printlns(
        "The available pruning strategies are legacy, support_independent, and "
        "partial_fd. The selected strategy determines the algorithm's usage scenario "
        "and has its own additional parameters, which are introduced in the "
        "corresponding scenarios below."
    )

    print(f"{YELLOW}>>> result_strategy.{RESET}")
    printlns(
        "This strategy acts as a filter over the final set of CFDs found according "
        "to the other algorithm parameters."
    )
    result_strategy_table = pd.DataFrame(
        [
            {
                "value": "direct",
                "filtering": "keeps every discovered CFD",
            },
            {
                "value": "lattice",
                "filtering": "keeps the most general CFDs for each RHS",
            },
            {
                "value": "tree",
                "filtering": "also keeps some specific high-support CFDs",
            },
        ]
    )
    print_table(result_strategy_table, "Result strategy variants:")

    auxiliary_parameter_table = pd.DataFrame(
        [
            {
                "parameter": "max_lhs",
                "meaning": "maximum number of attributes on the embedded FD's LHS",
            },
            {
                "parameter": "threads",
                "meaning": "number of worker threads",
            },
            {
                "parameter": "limit_pli_cache",
                "meaning": "maximum number of cached position-list indexes",
            },
            {
                "parameter": "rhs_indices",
                "meaning": "optional list of columns allowed on the RHS",
            },
        ]
    )
    print_table(auxiliary_parameter_table, "Auxiliary parameters:")


def print_dataset_description(df):
    banner("Dataset description")
    printlns(
        "The dataset contains fourteen bacteria-growth experiments. Oxygen level, "
        "temperature, pH, and nutrient level are controlled parameters; growth "
        "rate is the observed result. We use CFD mining to "
        "look for stable experimental regimes and possible design issues."
    )
    shown_df = df.copy()
    shown_df.insert(0, "#", range(1, len(shown_df) + 1))
    print_table(shown_df, "Bacteria-growth experiments:")


def run_legacy_scenario(df):
    banner("Scenario 1. Discovering CFDs with support and confidence thresholds")
    printlns(
        "The most straightforward way to use CFDFinder is to set minimum support "
        "and confidence thresholds. For this mode, select the legacy pruning "
        "strategy with pruning_strategy='legacy'. It returns only CFDs whose "
        "pattern tableaux meet both cfd_minsup and cfd_minconf."
    )
    print_table(parameters_to_dataframe(LEGACY_PARAMS), "Execution parameters:")

    cfds = mine_cfds(df, **LEGACY_PARAMS)
    printlns(
        f"CFDFinder discovered {len(cfds)} CFDs that meet both thresholds. "
        "Three useful examples are shown below."
    )

    featured_cfds = [
        (
            find_cfd(
                cfds,
                ["Oxygen_Level", "Nutrient_Level"],
                "Temperature_C",
            ),
            "For every oxygen level except High, the combination of oxygen and "
            "nutrient levels determines the experiment temperature. This rule "
            "reveals coupling between controlled parameters and suggests which "
            "oxygen-nutrient-temperature combinations are missing from the study.",
        ),
        (
            find_cfd(cfds, ["Growth_Rate"], "Temperature_C"),
            "For every growth regime except Very_Fast, each growth rate is "
            "associated with one temperature: Slow and Medium occur at 25 C, Fast "
            "at 35 C, and Dead at 40 C. This rule can help validate new measurements "
            "and identify operating regimes that deserve closer investigation.",
        ),
        (
            find_cfd(cfds, ["Temperature_C", "pH"], "Oxygen_Level"),
            "For every pH value except 7.5, temperature and pH determine the oxygen "
            "level. Because all three attributes are controlled parameters, this "
            "CFD is useful for detecting experimental-design bias and gaps in "
            "factor coverage.",
        ),
    ]

    for number, (cfd, explanation) in enumerate(featured_cfds, start=1):
        if cfd is None:
            continue
        print_cfd(cfd, number)
        printlns(explanation)

    return cfds


def run_support_independent_scenario(df):
    banner("Scenario 2. Rare but precise CFDs")
    printlns(
        "A high minimum support can hide rules that describe rare but informative "
        "rules. Such a rule may cover only a few rows yet be valuable when those "
        "rows follow it with high confidence. The support_independent strategy is "
        "designed for this case: it does not require a global cfd_minsup threshold."
    )
    print_table(parameters_to_dataframe(SUPPORT_INDEPENDENT_PARAMS), "Execution parameters:")

    printlns(
        "The min_support_gain=4 threshold requires every pattern to match at "
        "least four rows. With max_level_support_drop=2, support must not decrease "
        "by more than two rows when one attribute is removed from the embedded FD's "
        "LHS."
    )
    printlns(
        "The max_patterns=1 limit allows one row in each pattern tableau, max_lhs=2 "
        "allows at most two attributes on the embedded FD's LHS, and rhs_indices=[4] "
        "restricts the RHS to Growth_Rate."
    )

    cfds = mine_cfds(df, **SUPPORT_INDEPENDENT_PARAMS)
    printlns(
        f"CFDFinder discovered {len(cfds)} CFDs with Growth_Rate on the RHS. Two "
        "informative low-support rules are shown below."
    )

    featured_cfds = [
        (
            find_cfd(cfds, ["Oxygen_Level", "Temperature_C"], "Growth_Rate"),
            "The Temperature_C=[35 - 40] condition covers experiments #10-#14. "
            "Within this range, equal Oxygen_Level and Temperature_C values always "
            "have the same Growth_Rate: at 35 C, Medium oxygen corresponds to Fast "
            "growth and High oxygen to Very_Fast growth; at 40 C, the observed "
            "growth is Dead. The CFD has support 5/14≈0.3571 and confidence 1, so it "
            "identifies a small high-temperature region in which oxygen helps "
            "predict growth."
        ),
        (
            find_cfd(cfds, ["Temperature_C", "pH"], "Growth_Rate"),
            "The same Temperature_C=[35 - 40] range also makes temperature and pH "
            "a precise predictor of growth in experiments #10-#14. At 35 C, pH 7.0 "
            "corresponds to Fast growth and pH 7.5 to Very_Fast growth; at 40 C, the "
            "observed growth is Dead. This CFD also has support 5/14≈0.3571 and "
            "confidence 1, suggesting that pH is worth varying more thoroughly in "
            "future high-temperature experiments."
        ),
    ]

    for number, (cfd, explanation) in enumerate(featured_cfds, start=1):
        if cfd is None:
            continue
        print_cfd(cfd, number)
        printlns(explanation)

    printlns(
        "Because these CFDs are based on only five experiments, they are useful as "
        "signals and hypotheses rather than general scientific conclusions. More "
        "experiments in the 35-40 C range are needed to validate them."
    )
    return cfds


def run_partial_fd_scenario(df):
    banner("Scenario 3. Approximate dependencies covering the whole table")
    printlns(
        "The partial_fd strategy searches for dependencies that cover the entire "
        "relation but may have confidence below one. The max_g1 parameter bounds "
        "the fraction of tuple pairs that violate the embedded FD."
    )
    print_table(parameters_to_dataframe(PARTIAL_FD_PARAMS), "Execution parameters:")

    cfds = mine_cfds(df, **PARTIAL_FD_PARAMS)
    printlns(
        f"CFDFinder discovered {len(cfds)} approximate dependencies with "
        "Growth_Rate on the RHS. We consider one of them below."
    )

    verified_cfd = find_cfd(
        cfds,
        ["Oxygen_Level", "pH", "Nutrient_Level"],
        "Growth_Rate",
    )
    if verified_cfd is None:
        printlns("The partial FD selected for verification was not discovered.")
        return cfds

    print_cfd(verified_cfd)

    printlns(
        "CFDFinder reports the quality of a discovered partial FD but does not "
        "expose its violating rows. Since a verifier for CFDs under CFDFinder's "
        "definition is not yet available, we use the verifier for another algorithm "
        "to find the rows that violate the discovered CFD. Any partial FD has only "
        "wildcard bindings, so it can be passed directly to that verifier."
    )
    lhs_names, rhs_name = verified_cfd.embedded_fd.to_name_tuple()
    lhs_rule = [(column, "_") for column in lhs_names]
    rhs_rule = (rhs_name, "_")
    verifier = verify_cfd_rule(df, lhs_rule, rhs_rule, min_conf=1.0)
    violating_rows = collect_violating_rows(verifier)

    verification_result = pd.DataFrame(
        [
            {
                "CFD": format_embedded_fd(verified_cfd.embedded_fd),
                "holds": verifier.cfd_holds(),
                "violating rows": len(violating_rows),
            }
        ]
    )
    print_table(verification_result, "FD-First verification result:")

    if not violating_rows:
        printlns("The verifier found no violating rows.")
    else:
        comparison = df.loc[sorted(violating_rows)].copy()
        comparison.insert(0, "#", comparison.index + 1)
        print_table(comparison, "Rows marked as violations by the verifier:")
        printlns(
            "Rows #1, #2, #3, and #6 have the same Oxygen_Level, pH, and "
            "Nutrient_Level. Growth_Rate=Slow occurs twice in this LHS cluster, "
            "while Medium and Very_Fast occur once each. The verifier therefore "
            "marks rows #2 and #6 as violations. Such uncommon disagreements may be "
            "measurement errors, omitted factors, or evidence that the system is "
            "close to a stability boundary."
        )
    return cfds


def print_summary():
    banner("Summary")
    printlns(
        "You have now mastered the core idea behind CFDs and know how to apply it "
        "in practice."
    )
    printlns(
        "Using CFDFinder usually requires experimenting with strategies and "
        "their parameters. For straightforward scenarios with explicit minimum "
        "support and confidence thresholds, start with the legacy pruning strategy. "
        "Use support_independent when you need more flexible settings, especially "
        "for precise rules that cover small groups of rows."
    )
    printlns(
        "Use the RHS filter through rhs_indices when you are interested only in "
        "specific columns on the right-hand side. Restricting the allowed RHS "
        "columns can significantly reduce the algorithm's execution time."
    )
    printlns(
        "The negative_constant and range expansion strategies can substantially "
        "increase memory consumption and execution time because they generate many "
        "possible bindings. They are therefore best used when the attribute domains "
        "in the input table are not large."
    )


def print_see_also():
    banner("See also")
    print("Conditional Dependencies:")
    print("* CFD mining under definition [2]          - examples/basic/mining_cfd.py")
    print("* CFD verification under definition [2]    - examples/basic/verifying_cfd.py")
    print("* CIND mining under definition [3]         - examples/basic/mining_cind1.py")
    print("* CIND verification under definition [3]   - examples/basic/verifying_cind.py")
    print("* CIND mining under definition [4]         - examples/basic/mining_cind2.py")
    print()

    print("Functional Dependencies:")
    print("* AFD mining              - examples/basic/mining_fd_approximate.py")
    print("* FD mining               - examples/basic/mining_fd.py")
    print("* FD/AFD verification     - examples/basic/verifying_fd_afd.py")
    print()

def print_ref():
    banner("References:")
    printlns(
        "[1]  Efficient Discovery of Conditional Dependencies with Desbordante, "
        "39th FRUCT, Helsinki, Finland, 2026, pp. 130-141."
    )
    printlns(
        "[2]  J. Rammelaere and F. Geerts in 'Revisiting Conditional Functional Dependency"
        "Discovery: Splitting the “C” from the “FD”', ECML PKDD 2018."
    )
    printlns(
        "[3]  J. Bauckmann, Z. Abedjan, U. Leser, H. Muller, F. Naumann. Discovering Conditional Inclusion Dependencies. "
        "CIKM 2012, pp. 2094-2098."
    )
    printlns(
        "[4]  O. Cure. Improving the Data Quality of Drug Databases using"
        "Conditional Dependencies and Ontologies. ACM JDIQ 4(1):20, 2012."
    )

def main():
    print_intro()
    print_definitions()
    print_parameters()

    df = pd.read_csv(TABLE_PATH)
    print_dataset_description(df)
    run_legacy_scenario(df)
    run_support_independent_scenario(df)
    run_partial_fd_scenario(df)
    print_summary()
    print_see_also()
    print_ref()


if __name__ == "__main__":
    main()
