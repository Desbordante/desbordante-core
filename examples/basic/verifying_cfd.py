import desbordante
import pandas as pd

TABLE_PATH = 'examples/datasets/cfd_datasets/city.csv'
TABLE_PATH_FIXED = 'examples/datasets/cfd_datasets/city_fixed.csv'

class Colors:
    GREEN_BG = '\033[1;42m'
    GREEN_FG = '\033[1;32m'
    RED_BG = '\033[1;41m'
    RED_FG = '\033[1;31m'
    BLUE_FG = '\033[1;34m'
    YELLOW_BG = '\033[1;43m'
    DEFAULT_BG = '\033[1;49m'
    DEFAULT_FG = '\033[1;37m'
    RESET = '\033[0m'

CFD = desbordante.cfd.CFD

def explain_cfd_concept():
    """Explain what CFDs are with a real-world example."""
    print(f"{Colors.BLUE_FG}=== Understanding Conditional Functional Dependencies ==={Colors.RESET}\n")

    print("Conditional functional dependency (CFD) extends a classic functional")
    print("dependency by introducing additional conditions on attributes.")
    print()
    print("For example, consider employee data (a table), containing “Department”, “Head”, “Position”,")
    print("and “Location” attributes. In this case:")
    print()
    print("* An FD Department -> Head means that the head is unambiguously determined by the ")
    print("  department, indicating that each department (mentioned in the table) has exactly one head.")
    print()
    print("* On the other hand, a CFD: (Department=IT) -> (Head=Smith) means that the IT department")
    print("  has Smith as its head. Other departments may or may not follow suit.")
    print()
    print("* Finally, there is CFD with a wildcard: (Department=IT, Position=_) -> (Location=NYC). The")
    print("  wildcard symbol allows any values in its place. Therefore, this CFD means that any IT position,")
    print("  which is available in the company, is located in NYC.")
    print()

    print(f"{Colors.GREEN_FG}=== Formal Definition ==={Colors.RESET}")
    print()
    print("Conditional functional dependency is a very popular concept in the database community, and")
    print("there are several alternative definitions of CFD. In this example, we will rely on the definition")
    print("provided by J. Rammelaere and F. Geerts in 'Revisiting Conditional Functional Dependency")
    print("Discovery: Splitting the “C” from the “FD”', ECML PKDD 2018.")
    print()
    print("A template tuple t is a tuple where each attribute can either have:")
    print("* A fixed constant value")
    print("* A wildcard symbol ('_'), allowing for generalization across different data records.")
    print()

    print(f"{Colors.GREEN_FG}=== Key Metrics for CFD ==={Colors.RESET}")
    print("Support: The number of records satisfying the condition X (LHS part)")
    print("Confidence: The fraction of records where Y occurs given X")
    print()


def demonstrate_algorithm_parameters():
    print(f"{Colors.BLUE_FG}=== Algorithm Parameters Explanation ==={Colors.RESET}\n")

    print("CFD verification parameters:")
    print(f"* cfd_rule: the CFD to verify")
    print("  - CFD object with LHS and RHS")
    print("  - Required parameter")
    print("  - Example: CFD(lhs=[(0, 'Los Angeles'), (3, '_')], rhs=(4, 'high')), where 0,3,4 are attribute indices")
    print()

    print(f"* minconf: minimum confidence threshold")
    print("  - Lower values: more tolerant of violations")
    print("  - Range: 0.0 to 1.0")
    print()


def demonstrate_cfd_object_operations():
    """Demonstrate CFD object operations - comparison, sets, decomposition."""
    print(f"{Colors.BLUE_FG}=== CFD Object Operations ==={Colors.RESET}")
    print("CFDs are objects that support comparison, sets, and decomposition:")
    print()

    cfd1 = CFD(lhs=[(0, "Los Angeles")], rhs=(4, "high"))
    cfd2 = CFD(lhs=[(0, "Los Angeles")], rhs=(4, "high"))  # Same as cfd1
    cfd3 = CFD(lhs=[(0, "New York")], rhs=(4, "high"))
    cfd4 = CFD(lhs=[(3, "House")], rhs=(4, "high"))

    print(f"{Colors.GREEN_FG}1. String Representation:{Colors.RESET}")
    print(f"   CFD1: {cfd1}")
    print(f"   CFD2: {cfd2} {Colors.GREEN_FG}(same as CFD1){Colors.RESET}")
    print(f"   CFD3: {cfd3}")
    print(f"   CFD4: {cfd4}")
    print()

    print(f"{Colors.GREEN_FG}2. Decomposition:{Colors.RESET}")
    print(f"   CFD1 LHS: {cfd1.lhs}")
    print(f"   CFD1 RHS: {cfd1.rhs}")
    print(f"   First LHS item - attribute: {cfd1.lhs[0].attribute}, value: {cfd1.lhs[0].value}")
    print(f"   RHS - attribute: {cfd1.rhs.attribute}, value: {cfd1.rhs.value}")
    print()

    print(f"{Colors.GREEN_FG}3. Equality Comparison:{Colors.RESET}")
    print(f"   CFD1 == CFD2: {cfd1 == cfd2}")  # Should be True
    print(f"   CFD1 == CFD3: {cfd1 == cfd3}")  # Should be False
    print()

    print(f"{Colors.GREEN_FG}4. Set Operations:{Colors.RESET}")
    discovered_cfds = {cfd1, cfd2, cfd3, cfd4}  # cfd1 and cfd2 are duplicates

    print(f"   Discovered CFDs (duplicates removed): {len(discovered_cfds)} unique")
    for i, cfd in enumerate(sorted(discovered_cfds, key=str), 1):
        print(f"     {i}. {cfd}")
    print()


def create_and_explain_dataset():
    """Create a meaningful dataset and explain its contents."""
    print(f"{Colors.BLUE_FG}=== Dataset Explanation ==={Colors.RESET}\n")

    df = pd.read_csv(TABLE_PATH)

    print("Real estate dataset containing property information across major US cities:")
    print()
    print(df.to_string())
    print()

    print("Let's put forward some business hypotheses about this dataset: ")
    print("* Los Angeles properties should consistently have high building costs due to premium market")
    print("* All house-type properties should be expensive regardless of location")
    print("* New York apartments should command high prices in the competitive NYC market")
    print("* Apartment pricing may vary significantly across different cities")
    print()

    print("We'll use CFD verification to test these assumptions and identify any data anomalies.")
    print("This approach helps validate business rules and discover data quality issues.")
    print()


    return df


def print_table_with_highlight(table, indices_to_highlight):
    """Print table with highlighted rows"""
    if table.empty:
        print(table.to_string())
    else:
        df_as_string_lines = table.to_string().splitlines()
        print(df_as_string_lines[0])
        data_lines = df_as_string_lines[1:]

        for i, line_text in enumerate(data_lines):
            if table.index[i] in indices_to_highlight:
                print(f"{Colors.GREEN_BG}{line_text} (FIXED){Colors.RESET}")
            else:
                print(line_text)


def print_cfd_metrics(df, cfd, verifier):
    """Print CFD verification metrics and violations."""

    verifier.execute(cfd_rule=cfd, minconf=0)

    holds = verifier.cfd_holds()
    support = verifier.get_real_support()
    confidence = verifier.get_real_confidence()
    violations = verifier.get_num_rows_violating_cfd()

    print(f"CFD holds: {Colors.GREEN_FG if holds else Colors.RED_FG}{holds}{Colors.RESET}")
    print(f"Support: {support}")
    print(f"Confidence: {confidence:.2f}")
    print(f"Violations: {Colors.GREEN_FG if violations == 0 else Colors.RED_FG}{violations} rows{Colors.RESET}")
    print(f"Number of clusters violating FD: {verifier.get_num_clusters_violating_cfd()}")
    print()

    display_violations(df, verifier, cfd)


def display_violations(df, verifier, cfd):
    """Display detailed information about CFD violations."""
    highlights = verifier.get_highlights()
    if not highlights:
        return

    violating_rows = set()
    for highlight in highlights:
        violating_rows.update(highlight.get_violating_rows)

    if not violating_rows:
        return

    print(f"Violating rows: {Colors.RED_FG}{sorted(violating_rows)}{Colors.RESET}")

    lhs_items = cfd.lhs
    rhs_item = cfd.rhs

    for j, highlight in enumerate(highlights[:2], start=1):
        cluster_violating = highlight.get_violating_rows
        if cluster_violating:
            print(f"  Cluster #{j} violations:")
            for row_idx in list(cluster_violating)[:3]:
                lhs_values = [df.iloc[row_idx, item.attribute] for item in lhs_items]
                rhs_value = df.iloc[row_idx, rhs_item.attribute]
                print(f"    Row {row_idx}: {Colors.RED_FG}{lhs_values} -> {rhs_value}{Colors.RESET}")
    print()


def scenario_find_rhs_violations(df):
    """Scenario: Finding violations in RHS (right-hand side)"""
    print(f"{Colors.BLUE_FG}=== Scenario 1: Finding RHS Violations ==={Colors.RESET}")
    print("Let's verify our first hypothesis: Los Angeles properties should consistently have high building costs.")
    print()

    verifier = desbordante.cfd_verification.algorithms.Default()
    verifier.load_data(table=df)

    cfd_to_verify = CFD(
        lhs=[(0, "Los Angeles")],
        rhs=(4, "high")
    )

    print("CFD to verify: [City='Los Angeles'] -> BuildingCost='high'")
    print_cfd_metrics(df, cfd_to_verify, verifier)

    print("Analysis: Our hypothesis is violated! There is one LA property with a non-high cost.")
    print("This suggests either data quality issues or exceptions to the premium market rule.")
    print()
    print("Let's investigate and fix potential data quality issues:")


    fix_condition = (df["City"] == "Los Angeles") & (df["BuildingCost"] != "high")
    indices_to_highlight = df[fix_condition].index.tolist()

    print(f"Found {len(indices_to_highlight)} LA properties with non-high costs:")
    for idx in indices_to_highlight:
        row = df.iloc[idx]
        print(f"  {idx}: " + ", ".join(map(str, row.values)))

    print()
    print("Assuming these are data entry errors, let's correct them:")
    print()

    df_fixed = df.copy()
    df_fixed.loc[fix_condition, "BuildingCost"] = "high"

    verifier = desbordante.cfd_verification.algorithms.Default()
    verifier.load_data(table=df_fixed)
    print_table_with_highlight(df_fixed, indices_to_highlight)
    print_cfd_metrics(df_fixed, cfd_to_verify, verifier)

    print("CFD now holds perfectly after correction.")
    print("All Los Angeles properties now have 'high' building costs as expected.")
    print("The violation was indeed a data entry error, not a business exception.")
    print()

    print("This example demonstrates CFDs as powerful data quality tools:")
    print("* Automatically identify inconsistent data patterns")
    print("* Spot outliers that violate business rules")
    print("* Find encoding errors (e.g., 'Low' vs 'low' vs 'LOW')")
    print()


def scenario_find_lhs_violations(df):
    """Scenario: Finding violations by examining LHS conditions"""
    print(f"{Colors.BLUE_FG}=== Scenario 2: Analyzing Different LHS Conditions ==={Colors.RESET}")

    print("In this scenario, we'll explore how different LHS (Left-Hand Side) conditions")
    print("affect CFD validation.")
    print()

    verifier = desbordante.cfd_verification.algorithms.Default()
    verifier.load_data(table=df)

    print("Let's systematically test our business hypotheses, moving from specific to general conditions.")
    print()

    # Hypothesis 2: All house-type properties should be expensive
    print(f"{Colors.GREEN_FG}Hypothesis 2: Market Premium for Houses{Colors.RESET}")
    print("Business Logic: Houses typically require more land and construction, commanding premium prices.")
    print()

    cfd_house = CFD(
        lhs=[(3, "House")],  # BuildingType='House'
        rhs=(4, "high")      # BuildingCost='high'
    )

    print("CFD to verify: [BuildingType='House'] -> BuildingCost='high'")
    print("If a property is a house, then its building cost should be high")
    print()
    print_cfd_metrics(df, cfd_house, verifier)

    print("Analysis: Our house pricing hypothesis is confirmed.")
    print("This CFD holds because houses consistently have high costs across all locations.")
    print()

    # Hypothesis 3: New York apartments should command high prices
    print(f"{Colors.GREEN_FG}Hypothesis 3: Location + Type Specificity{Colors.RESET}")
    print("Business Logic: NYC's competitive market + apartment density = premium pricing.")
    print()

    cfd_ny_apartment = CFD(
        lhs=[(0, "New York"), (3, "Apartment")],  # City='New York' AND BuildingType='Apartment'
        rhs=(4, "high")
    )

    print("CFD to verify: [City='New York' AND BuildingType='Apartment'] -> BuildingCost='high'")
    print("If a property is an apartment in New York, then its cost should be high")
    print_cfd_metrics(df, cfd_ny_apartment, verifier)

    print("Analysis: This demonstrates how combining conditions creates stronger rules.")
    print("Multiple LHS conditions (City + BuildingType) create more precise CFDs.")
    print()

    # Hypothesis 4: Test generalization - what happens when we remove location constraint?
    print(f"{Colors.YELLOW_BG}{Colors.DEFAULT_FG}Hypothesis 4: Testing Generalization Effects{Colors.RESET}")
    print("Business Logic: Do apartments universally command high prices, or is location crucial?")
    print()

    cfd_apartment_general = CFD(
        lhs=[(3, "Apartment")],  # BuildingType='Apartment' only
        rhs=(4, "high")
    )

    print("CFD to verify: [BuildingType='Apartment'] -> BuildingCost='high'")
    print("If a property is an apartment, then its cost should be high")
    print_cfd_metrics(df, cfd_apartment_general, verifier)

    print("Analysis: This CFD violation reveals important patterns!")
    print("  * Root cause: Location matters - different cities have different apartment pricing")
    print("  * Evidence: 4 violations suggest Chicago/other cities have lower apartment costs")
    print("  * Lesson: Single-attribute conditions can be too general for real-world data")
    print("  * Solution: Either add City constraints OR accept ~56% confidence for general trends")
    print()


    print(f"{Colors.BLUE_FG}=== Learning Summary: LHS Condition Design ==={Colors.RESET}")
    print("Key lessons from this scenario:")
    print()
    print("1. SINGLE CONDITIONS can work when the attribute strongly predicts the outcome")
    print("   Example: [BuildingType='House'] -> Cost='high'")
    print()
    print("2. MULTIPLE CONDITIONS create more precise, reliable rules")
    print("   Example: [City='New York' AND BuildingType='Apartment'] -> Cost='high'")
    print()
    print("3. OVERGENERALIZATION often leads to violations")
    print("   Example: [BuildingType='Apartment'] -> Cost='high'")
    print()
    print("4. BUSINESS CONTEXT should guide CFD design")
    print("   * Consider market factors, geographic differences, property characteristics")
    print("   * Start specific, then test if generalization is possible")
    print()

    print("Next steps in real applications:")
    print("* Experiment with wildcards (_) for partial generalization")
    print("* Test different confidence thresholds for noisy data")
    print("* Use violation analysis to discover hidden business rules")
    print()


def demonstrate_experimentation_workflow():
    """Show how CFD validation requires experimentation."""
    print(f"{Colors.BLUE_FG}=== Experimentation Workflow ==={Colors.RESET}")

    print("CFD validation typically requires experimentation to find meaningful patterns:")
    print("1. Start with strict confidence (1.0) to find exact violations")
    print("2. Lower confidence gradually to handle noisy data")
    print("3. Use wildcards (_) strategically to generalize patterns")
    print("4. Combine domain knowledge with statistical validation")
    print("5. Validate CFDs incrementally as you clean your data")
    print()

    print("Performance considerations:")
    print("* Larger datasets may require sampling for exploration")
    print("* Complex CFDs with many conditions may be slower")
    print("* Consider validating subsets of your data first")
    print()


def main():
    print(f"{Colors.BLUE_FG}CFD Validation Example - Desbordante{Colors.RESET}\n")

    explain_cfd_concept()

    demonstrate_algorithm_parameters()
    demonstrate_cfd_object_operations()

    df = create_and_explain_dataset()
    scenario_find_rhs_violations(df)

    df = pd.read_csv(TABLE_PATH)
    scenario_find_lhs_violations(df)

    demonstrate_experimentation_workflow()

    print(f"{Colors.GREEN_FG}CFD verifying example completed!{Colors.RESET}")
    print()


if __name__ == '__main__':
    main()
