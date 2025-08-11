import desbordante
import pandas as pd
import time

MINIMUM_SUPPORT = 7      # Minimum number of tuples that must satisfy the LHS pattern
MINIMUM_CONFIDENCE = 0.75 # Minimum ratio of tuples satisfying both LHS and RHS patterns
MAXIMUM_LHS_COUNT = 2    # Maximum number of attributes in the LHS

TABLE_PATH = 'examples/datasets/cfd_datasets/employees.csv'

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
    print("  wildcard symbol allows any values in its place. Therefore, this CFD means that any IT position")
    print("  in the company is available in NYC.")
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
    """Show how different parameters affect CFD discovery."""
    print(f"{Colors.BLUE_FG}=== Algorithm Parameters Explanation ==={Colors.RESET}\n")

    print("CFD mining parameters:")
    print(f"* cfd_minsup (minimum support):")
    print("  - Range: 1 to number of rows in dataset")
    print()

    print(f"* cfd_minconf (minimum confidence):")
    print("  - Range: 0.0 to 1.0")
    print()

    print(f"* cfd_max_lhs (maximum LHS size):")
    print("  - Maximum number of attributes in left-hand side")
    print("  - Range: 1 to number of columns")
    print()


def show_available_algorithms():
    """Display information about available CFD algorithms."""
    print(f"{Colors.BLUE_FG}=== Available CFD Algorithms ==={Colors.RESET}\n")

    print("Desbordante provides the following CFD mining algorithm:")
    print("* FDFirst (by J. Rammelaere and F. Geerts): The primary algorithm for CFD discovery")
    print("  - Based on the FD-first approach")
    print("  - Discovers FDs first, then adds conditions")
    print("  - Efficient for datasets with clear functional relationships")
    print()

    algo = desbordante.cfd.algorithms.Default()
    print(f"Default algorithm: {type(algo).__name__}")
    print()


def create_and_explain_dataset():
    """Create a meaningful dataset and explain its contents."""
    print(f"{Colors.BLUE_FG}=== Dataset Explanation ==={Colors.RESET}\n")

    df = pd.read_csv(TABLE_PATH)

    print("Let’s consider an example dataset containing employee data.")
    print()
    print(df.to_string())
    print()

    print("Expected patterns in this dataset:")
    print("1. We can see that each position has the same salary level. This can be expressed in the")
    print("   following CFD: Position -> Salary. All managers have high salaries, all developers ")
    print("   have average salaries, etc.")
    print("2. We might expect certain positions to be department-specific, but examining the data reveals")
    print("   that Managers work across multiple departments (IT, HR, Finance). This violates the potential")
    print("   CFD: Position -> Department, showing that management roles span organizational boundaries.")
    print("3. Looking at the data, one can note that all HR specialists reside in LA, except for one. This")
    print("   fact can be expressed by the following CFD: Department=HR -> Location=LA. However, an HR")
    print("   specialist in Boston (row 6) violates this location pattern. Therefore, this CFD does not hold in")
    print("   the dataset.")
    print()

    return df


def advanced_cfd_analysis(df):
    """Perform detailed analysis of discovered CFDs."""
    print(f"{Colors.BLUE_FG}=== Detailed CFD Analysis ==={Colors.RESET}\n")

    print("Now let's discover all conditional functional dependencies in our dataset...")
    print()

    print(f"Running CFD mining with the following parameters:")
    print(f"* Minimum Support (cfd_minsup): {MINIMUM_SUPPORT}")
    print(f"* Minimum Confidence (cfd_minconf): {MINIMUM_CONFIDENCE}")
    print(f"* Maximum LHS Count (cfd_max_lhs): {MAXIMUM_LHS_COUNT}")
    print()

    algo = desbordante.cfd.algorithms.Default()
    algo.load_data(table=df)
    algo.execute(
        cfd_minsup=MINIMUM_SUPPORT,
        cfd_minconf=MINIMUM_CONFIDENCE,
        cfd_max_lhs=MAXIMUM_LHS_COUNT
    )

    cfds = algo.get_cfds()

    print(f"Discovered {len(cfds)} CFDs:")
    for i, cfd in enumerate(cfds, 1):
        print(f"  {i}. {cfd}")
    print()

    if not cfds:
        print("No CFDs discovered with current parameters.")
        return

    column_names = df.columns.tolist()
    print(f"Column mapping: {dict(enumerate(column_names))}")
    print()

    target_cfds = [
        "{(1, _)} -> (3, _)",  # Position -> Salary
        "{(1, _)} -> (0, _)",  # Position -> Department
        "{(0, _)} -> (2, _)"   # Department -> Location
    ]

    selected_cfds = []
    for cfd in cfds:
        cfd_str = str(cfd)
        if any(target in cfd_str for target in target_cfds):
            selected_cfds.append(cfd)

    print("Let's analyze some of the discovered CFDs:")
    print()

    verifier = desbordante.cfd_verification.algorithms.Default()
    verifier.load_data(table=df)


    for i, cfd in enumerate(selected_cfds, 1):
        print(f"{Colors.GREEN_FG}CFD: {cfd}{Colors.RESET}")

        print_cfd_metrics(df, cfd, verifier)

        cfd_str = str(cfd)

        if "{(1, _)} -> (3, _)" in cfd_str:
            print("Analysis: This CFD represents Position -> Salary dependency.")
            print("As we expected, each position has a consistent salary level.")
            print("All Managers earn High salary, Developers get Medium, and Specialists receive Low.")
            print("This confirms our compensation structure is working correctly - no violations found.")

        elif "{(1, _)} -> (0, _)" in cfd_str:
            print("Analysis: This CFD represents Position -> Department dependency.")
            print("We suspected this might not hold perfectly, and indeed it doesn't.")
            print("The violations show that Managers work across multiple departments (IT, HR, Finance).")
            print("This actually makes business sense - management roles often span organizational boundaries.")

        elif "{(0, _)} -> (2, _)" in cfd_str:
            print("Analysis: This CFD represents Department -> Location dependency.")
            print("Just as we suspected - there's likely a data entry error here.")
            print("The violation shows an HR employee in Boston, but we expected all HR staff in LA.")
            print("This Boston HR specialist is probably a mistake that needs to be corrected.")

        else:
            print("Analysis: This CFD represents a functional relationship between employee attributes.")
            print("Such patterns help identify business rules and data constraints.")

        print()

    print("Note: These CFDs use wildcards (_) indicating they apply to any value in those positions.")
    print("This happens because our dataset is small (8 rows) and we're using relatively low thresholds.")
    print("On larger datasets with higher confidence/support thresholds, you'd see more specific")
    print("CFDs with fixed constant values like (Department=IT) -> (Location=NYC).")
    print()


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


def demonstrate_experimentation_workflow():
    """Show how CFD mining requires experimentation."""
    print(f"{Colors.BLUE_FG}=== Experimentation Workflow ==={Colors.RESET}\n")

    print("CFD mining typically requires experimentation to find meaningful patterns:")
    print("1. Start with conservative parameters (high confidence, low support)")
    print("2. Examine discovered CFDs for domain relevance")
    print("3. Adjust parameters based on findings:")
    print("   - Too few CFDs? Lower confidence or support")
    print("   - Too many CFDs? Raise thresholds or limit LHS size")
    print("   - Irrelevant patterns? Focus on specific attributes")
    print("4. Use CFDs to identify and fix data quality issues")
    print("5. Re-run mining on cleaned data for better patterns")
    print()

    print("This iterative process helps discover:")
    print("* Business rules encoded in the data")
    print("* Data quality issues and outliers")
    print("* Constraints that should be enforced")
    print()


def main():
    """Main function demonstrating all CFD capabilities."""
    print(f"{Colors.BLUE_FG}CFD Mining Example - Desbordante {Colors.RESET}\n")

    explain_cfd_concept()

    show_available_algorithms()

    demonstrate_algorithm_parameters()

    df = create_and_explain_dataset()

    advanced_cfd_analysis(df)

    demonstrate_experimentation_workflow()

    print(f"{Colors.GREEN_FG}CFD mining example completed!{Colors.RESET}")
    print()
    print(f"{Colors.GREEN_FG}See verifying_cfd.py for CFD validation examples.{Colors.RESET}")
    print()

if __name__ == '__main__':
    main()
