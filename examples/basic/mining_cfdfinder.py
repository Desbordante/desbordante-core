import desbordante
import pandas as pd

MINIMUM_SUPPORT = 0.8
MINIMUM_CONFIDENCE = 1
MAXIMUM_LHS_COUNT = 2
MIN_SUPPORT_GAIN = 4
MAX_SUPPORT_DROP = 2
MAX_PATTERNS = 1
RHS_INDICES = [4]
MAXIMUM_G1 = 0.02

BACTERIA_TABLE_PATH = 'examples/datasets/bacteria.csv'
bacteria_df = pd.read_csv(BACTERIA_TABLE_PATH)

class Colors:
    GREEN_FG = '\033[1;32m'
    BLUE_FG = '\033[1;34m'
    RESET = '\033[0m'


def explain_cfd_concept():
    """Explain what CFDs are with a real-world example."""
    print(f"{Colors.BLUE_FG}=== Explain Conditional Functional Dependencies ==={Colors.RESET}\n")
    
    print("Conditional Functional Dependencies (CFD) generalize traditional functional dependencies (FD)")
    print("by adding conditions to attribute values via the pattern tableau.")
    print("This allows you to find dependencies that are not performed for the entire table,")
    print("but only for a subset of rows that meet certain conditions in the data.")
    print()
    
    print(f"{Colors.GREEN_FG}=== Formal Definition ==={Colors.RESET}")
    print()
    print("This example examines the CFDFinder algorithm, which uses the following definition of CFD:")
    print()
    print(f"CFD is a pair of embedded FD and a pattern tableau:")
    print("* Embedded FD (X -> A) is a common functional dependency,")
    print("  where X (LHS) is a subset of table attributes and A (RHS) is a single attribute not contained in X.")
    print()
    print("* The pattern tableau is a table with attributes LHS and RHS in which each tuple contains")
    print("  one of the following types of conditions in each of its attributes:")
    print("    - A fixed constant value. Example: 'London'")
    print("    - A fixed negative value. Example: '¬Mark' or '¬30'")
    print("    - A range of values. Example: '[30 - 65]'")
    print("    - A wildcard symbol ('_') that allows any condition in this attribute.")
    print()

    print("For example, consider medical data (a table) containing the attributes Diagnosis,")
    print("Genetic_Marker, Blood_Type:")
    print("   * FD [Diagnosis, Blood_Type] -> [Genetic_Marker] may mean that people with the same")
    print("     diagnosis and blood type always have the same genetic marker.")
    print()
    print("   * On the other hand, CFD [Diagnosis, Blood_Type] -> [Genetic_Marker] with pattern")
    print("     tableau {(Cancer|_), (Diabetes|_)} clarifies that the previous rule applies only")
    print("     when Cancer and Diabetes are diagnosed.")
    print()
    
    print(f"{Colors.GREEN_FG}=== Note ===")
    print("CFDFinder searches for CFDs, where for RHS there is always a '_' in the condition,")
    print("that is, the wildcard symbol will always be in the pattern tableau for attribute A")
    print(f"(therefore, the pattern tableau does not contain attribute A).{Colors.RESET}")
    print()

    print(f"{Colors.GREEN_FG}=== Key Quality Measures for CFD ==={Colors.RESET}")
    print()
    print("Support: The fraction of records satisfying the condition LHS")
    print("Confidence: The fraction of records where RHS occurs given LHS")
    print()


def demonstrate_basic_parameters():
    print(f"{Colors.BLUE_FG}=== Algorithm Basic Parameters ==={Colors.RESET}\n")
    
    print("The CFDFinder algorithm supports several use cases, and each of them has its own parameters.")
    print("To begin with, we will describe the parameters common to all scenarios.")
    print()

    print("* cfd_max_lhs: The maximum number of attributes in LHS")
    print("  - Range: from 1 to number of columns")
    print()

    print("* pli_cache_limit: The maximum number of cached plis that are used in the study")
    print("  of candidates with similar LHS")
    print("  - Range: from 1 to infinity (if memory permits)")
    print()

    print(f"* expansion_strategy: Defines which types of conditions can be used in the pattern tuple.")
    print("  - 'constant': Only constants and wildcard are used as conditions for attributes.")
    print()
    print("  - 'negative_constant': Similar to the 'constant' strategy, but the negation")
    print("    condition is added.")
    print()
    print("  - 'range': The condition for each attribute is represented by a range of constant.")
    print("    To do this, the attribute domain is arranged in lexicographic order.")
    print("    Also in this strategy, the conditions may have the usual constants and wildcard symbol.")
    print()

    print(f"* result_strategy: Defines the form in which the result of the algorithm will be obtained.")
    print("  - 'direct': The result of the algorithm will be all the CFDs found according to")
    print("    the specified parameters.")
    print()
    print("  - 'lattice': Of all the discovered CFDs, only the most general of those rules")
    print("    that have the same RHS will be included in the result.")
    print("    For example, if the algorithm finds rules [X, Y, Z] -> A, [X, Y] -> A,")
    print("    [Y, Z] -> A, [Y] -> A and [X] -> A, then only rule [X] -> A and [Y] -> A")
    print("    will be included in the result.")
    print()
    print("  - 'tree': It works similarly to the 'lattice' strategy, but it can also identify")
    print("    additional specific CFDs with high support, which are sometimes lost in strict generalization.")
    print()

    print(f"* pruning_strategy: Defines the various use cases of the algorithm that will be discussed further.")
    print("   - Possible values: ['legacy', 'support_independent', 'rhs_filter', 'partial_fd']")
    print()


def create_and_explain_bacteria_dataset():
    """Create a meaningful dataset and explain its contents."""
    print(f"{Colors.BLUE_FG}=== Bacteria Dataset Explanation ==={Colors.RESET}\n")
    
    print("Let's look at an example of a dataset containing the results of experiments on")
    print("growing a strain of bacteria:")
    print()
    print(bacteria_df.to_string())
    print()

    print("We take oxygen level, temperature, pH and nutrient level as study parameters,")
    print("and measure the growth rate and number of populations as a result.")
    print()

    print("For knowledge discovery and data quality assessment, it is")
    print("interesting for us to study these results in order to draw some conclusions")
    print("and determine the direction of the next experiments.")
    print()
    
    print("For example, we may be interested in the following:")
    print()
    print("* 1. Have we chosen the values of the study parameters correctly?")
    print("     Are the parameters themselves independent?")
    print()
    print("* 2. Which parameters introduce instability in growth rate predictions?")
    print()
    print("* 3. Which parameter values are the boundary values for the stability of the results.")
    print()

    print("Let's try to answer these questions using the CFD mining.")
    print()


def demonstrate_legacy_strategy():
    """Show legacy pruning strategy."""
    print(f"{Colors.BLUE_FG}=== Legacy Strategy ==={Colors.RESET}\n")
    
    print("One of the possible scenarios for using the algorithm is to search for CFDs with")
    print("minimal support and confidence thresholds. To do this, you can use the 'legacy'")
    print("pruning strategy, which takes the following parameters:")
    print()
    print("* cfd_minconf (minimal confidence):")
    print("  - Range: from 0.0 to 1.0")
    print()
    print("* cfd_minsup (minimal support):")
    print("  - Range: from 0.0 to 1.0")
    print()

    print("Let's run the algorithm with the following parameters:")
    print()
    print(f"  * Pruning Strategy (pruning_strategy): legacy")
    print(f"  * Expansion Strategy (expansion_strategy): negative_constant")
    print(f"  * Result Strategy (result_strategy): direct")
    print(f"  * Minimum Support (cfd_minsup): {MINIMUM_SUPPORT}")
    print(f"  * Minimum Confidence (cfd_minconf): {MINIMUM_CONFIDENCE}")
    print()

    algo = desbordante.cfd.algorithms.CFDFinder()
    algo.load_data(table=bacteria_df)

    algo.execute(
        expansion_strategy="negative_constant",
        pruning_strategy="legacy",
        result_strategy="direct",
        cfd_minsup=MINIMUM_SUPPORT,
        cfd_minconf=MINIMUM_CONFIDENCE,
        max_lhs=MAXIMUM_LHS_COUNT
    )
    cfds = algo.get_cfds()

    print(f"With our parameters, the algorithm detected {len(cfds)} CFDs.")
    print()
    print("Let's look at the results that express the dependence between the study parameters.")
    print()

    result_cols = {4, 5}  # 'Growth_Rate' and 'Population_Count' column indices
    target_cfds = [target_cfd for target_cfd in cfds
                   if not set(target_cfd.embedded_fd.lhs_indices) & result_cols]

    for i, cfd in enumerate(target_cfds, 1):
        print(f"  {i}. {cfd}")
        print(f"    Support: {cfd.support}")
        print(f"    Confidence: {cfd.confidence}")
        print()

        if (cfd.embedded_fd.lhs_indices == [0, 3] and
            cfd.embedded_fd.rhs_index == 1 and
            cfd.tableau == [['¬High', '_']]):
            print("  This CFD indicates that:")
            print("  If the oxygen level is not 'High' and nutrient level has the same values")
            print("  then according to our data we can predict the temperature under which")
            print("  the experiment was conducted.")
            print()
    
    print("This dependence may indicate an error in the design of the experiments,")
    print("since temperature is not an independent variable.")
    print("It may be worth conducting additional experiments with more random parameters.")
    print()


def demonstrate_support_independent_strategy():
    """Show how CFDFinder mining partial FDs."""
    print(f"{Colors.BLUE_FG}=== Support Independent and RHS Filter Strategies ==={Colors.RESET}\n")
    
    print("Sometimes it is still useful to look for low-support dependencies, as they can")
    print("identify rare but interesting dependencies that are performed on a small group of records.")
    print("To find them, you can use an interactive selection of minimum support values in")
    print("the 'legacy' strategy, but this approach is not optimal.")
    print("The 'support independent' strategy is better suited for such cases.")
    print()

    print("This strategy has the following parameters:")
    print()
    print("* cfd_minconf (minimal confidence):")
    print("  - Range: from 0.0 to 1.0")
    print()
    print("* min_support_gain: the minimum number of tuples that each pattern in the")
    print("  pattern tableau must support.")
    print("  - Range: from 1 to number of rows")
    print()
    print("* max_support_drop: the maximum number of tuples by which CFD support can")
    print("  decrease when one attribute from LHS is removed from the embedded FD.")
    print("  - Range: from 1 to number of rows")
    print()
    print("* max_patterns: maximum number of rows in the pattern tableau.")
    print("  - Range: from 1 to number of rows")
    print()

    print("If we are only interested in those rules that express dependence for certain")
    print("attributes, then to reduce the running time of the algorithm, we can use the")
    print("'rhs_filter' strategy, which is an extension of the previous strategy and")
    print("adds another one to all previous parameters:")
    print()
    print("* rhs_indices: the indexes of the attributes we are interested in for the RHS.")
    print("  - Example: [1,3,5]")
    print()

    print("Let's run the algorithm with the following parameters:")
    print()
    print(f"  * Pruning Strategy (pruning_strategy): rhs_filter")
    print(f"  * Expansion Strategy (expansion_strategy): range")
    print(f"  * Result Strategy (result_strategy): lattice")
    print(f"  * Minimum Confidence (cfd_minconf): {MINIMUM_CONFIDENCE}")
    print(f"  * Minimum support gain (min_support_gain): {MIN_SUPPORT_GAIN}")
    print(f"  * Maximum support drop (max_support_drop): {MAX_SUPPORT_DROP}")
    print(f"  * Maximum patterns (max_patterns): {MAX_PATTERNS}")
    print(f"  * RHS indices (rhs_indices): {RHS_INDICES}")
    print()

    algo = desbordante.cfd.algorithms.CFDFinder()
    algo.load_data(table=bacteria_df)

    algo.execute(
        expansion_strategy="range",
        pruning_strategy="rhs_filter",
        result_strategy="lattice",
        cfd_minconf=MINIMUM_CONFIDENCE,
        min_support_gain=MIN_SUPPORT_GAIN,
        max_support_drop=MAX_SUPPORT_DROP,
        max_patterns=MAX_PATTERNS,
        rhs_indices=RHS_INDICES
    )

    cfds = algo.get_cfds()
    print(f"Discovered {len(cfds)} CFDs:")
    print()
    
    print("Let's say we're interested in the effect of temperature on growth rate.")
    print("Consider the following low-support dependencies:")
    print()
    
    interesting_lhs = {1}
    low_support_cfds = [cfd for cfd in cfds
                        if (cfd.support <= 0.4) and
                        any(index in interesting_lhs for index in cfd.embedded_fd.lhs_indices)]

    for i, cfd in enumerate(low_support_cfds, 1):
        print(f"  {i}. {cfd}")
        print(f"    Support: {cfd.support}")
        print(f"    Confidence: {cfd.confidence}")
        print()
        if (cfd.embedded_fd.lhs_indices == [0, 1] and
            cfd.tableau == [['_', '[35 - 40]']]):
            print("  This CFD indicates that:")
            print("  At a temperature of 35-40 degrees, the combination of oxygen regime")
            print("  and temperature uniquely determines the growth rate.")
        elif (cfd.embedded_fd.lhs_indices == [1, 2] and
              cfd.tableau == [['[35 - 40]', '_']]):
            print("  This CFD indicates that:")
            print("  In the same temperature range of 35-40 degrees, the combination of")
            print("  temperature and pH also uniquely determines growth.")
        print()

    print("Both rules are useful, despite the low support, because they identify a")
    print("temperature zone 35-40 degrees where the system becomes as predictable as possible")
    print("and where you can focus on one key parameter oxygen level or pH instead of")
    print("controlling all factors at the same time.")
    print()


def demonstrate_partial_fd_extension():
    """Show how CFDFinder can be used to find partial FDs."""
    print(f"{Colors.BLUE_FG}=== Partial FD Strategy ==={Colors.RESET}\n")
    
    print("In addition to searching for common CFDs, the CFDFinder algorithm can be used")
    print("to mine partial FDs.")
    print()

    print(f"{Colors.GREEN_FG}=== Formal Definition ==={Colors.RESET}")
    print()
    print("Partial FD is a CFD covering the entire relation instance, i.e. those that")
    print("have a support of 1, but do have a confidence of less than 1.")
    print()

    print("To search, you can use the 'partial_fd' pruning strategy, which has a single")
    print("parameter 'max_g1'. g1 from the G family of metrics determines the fraction")
    print("of pairs of records violating embedded FD.")
    print()

    print("For example, let's run the algorithm on our bacteria dataset with the")
    print("following parameters:")
    print()
    print(f"  * pruning_strategy: partial_fd")
    print(f"  * result_strategy: lattice")
    print(f"  * max_g1: {MAXIMUM_G1}")
    print()

    algo = desbordante.cfd.algorithms.CFDFinder()
    algo.load_data(table=bacteria_df)
    
    algo.execute(
        expansion_strategy="constant",
        pruning_strategy="partial_fd",
        result_strategy="lattice",
        max_g1=MAXIMUM_G1
    )

    partial_fds = algo.get_cfds()

    print(f"Discovered {len(partial_fds)} partial FDs:")
    print()
    print("Let's look at the rules that have the 'Growth_Rate' attribute in the dependency")
    print("defining attribute.")
    print()

    interesting_rhs = {4}
    interesting_partial_fds = [partial_fd for partial_fd in partial_fds 
                               if partial_fd.embedded_fd.rhs_index in interesting_rhs]

    for i, partial_fd in enumerate(interesting_partial_fds, 1):
        print(f"  {i}. {partial_fd}")
        print(f"    Support: {partial_fd.support}")
        print(f"    Confidence: {partial_fd.confidence}")
        print()
    
    print("Let's see which entries violate the embedded FDs of these dependencies.")
    print("For example, take the following pair of records:")
    print()
    print(bacteria_df.iloc[[1, 2]])
    print()

    print("That is, with the same dependency parameters, the growth rate is rare,")
    print("but it can be different. Perhaps this is an error in the data itself,")
    print("or the values of the parameters at which the violation occurs, however,")
    print("are near stability boundaries.")
    print("Also, the dependencies differ only in the attributes Oxygen_Level and")
    print("Temperature_C, which may indicate a strong correlation of these parameters")
    print("or similar information regarding Growth_Rate.")
    print()

    print("In any case, the analysis helped us to draw some conclusions about our data")
    print("and direct it to further experiments.")
    print()

    print(f"{Colors.GREEN_FG}=== Note ===")
    print("For more information about mining partial fd with the g1 metric, we recommend")
    print(f"reading the example 'mining_afd.py'.{Colors.RESET}")
    print()


def demonstrate_experimentation_workflow():
    """Show how CFD mining requires experimentation."""
    print(f"{Colors.BLUE_FG}=== Experimentation Workflow ==={Colors.RESET}\n")
    
    print("When searching for CFDs, it is usually necessary to experiment with the algorithm")
    print("parameters, since it is quite difficult to immediately get the right set for you.")
    print()

    print("If your goal is to simplely find high-support CFDs, the legacy strategy is well suited.")
    print("Recommendations for selecting parameters:")
    print("1. Start with middle support and high confidence")
    print("2. If there are few CFDs, loosen the restrictions")
    print("3. If there are a lot of CFDs, increase the LHS size limit or use lattice/tree result strategy.")
    print()

    print("If your goal is to find rare but interesting CFDs, the support independent strategy is well suited.")
    print("Recommendations for selecting parameters:")
    print("1. The parameter max_support_drop and min_support_gain strongly influence the number of discovered CFDs.")
    print("   Their values should be selected in the range from '1' value to 30 percent of the number of rows in the table.")
    print("2. Maximum patterns strongly affects the running time of the algorithm, so start with small values (for example, 2 or 3).")
    print("   This way, a tableau with a small number of patterns will be more concise.")
    print("3. Use the rhs_filter strategy if you are only interested in certain attributes for the RHS.")
    print("   This will significantly reduce the running time of the algorithm.")
    print()

    print("If your goal is to look for errors and inaccuracies in the data, a partial FDs search may be")
    print("well suited for you. You can start with small values of the max g1 parameter (for example 0.01)")
    print("and increase if necessary.")
    print()

    print("Such experiments on your data will help you discover:")
    print("  * new hidden knowledge")
    print("  * data quality issues")
    print("  * patterns useful for decision making")
    print()


def main():
    """Main function demonstrating CFDFinder algorithm."""
    print(f"{Colors.BLUE_FG}CFDFinder Mining Example - Desbordante{Colors.RESET}\n")
    
    print(f"{Colors.GREEN_FG}=== Note ===")
    print("CFD is a unique pattern for which there are several search algorithms,")
    print("each of which defines CFD in its own way.")
    print("Therefore, we suggest that you review all the examples of the CFD pattern")
    print("and choose your appropriate option depending on your task:")
    print()
    print("1. verifying_cfd.py")
    print("2. mining_cfd.py")
    print(f"{Colors.RESET}")
    
    explain_cfd_concept()
    demonstrate_basic_parameters()
    create_and_explain_bacteria_dataset()
    demonstrate_legacy_strategy()
    demonstrate_support_independent_strategy()
    demonstrate_partial_fd_extension()
    demonstrate_experimentation_workflow()
    
    print(f"{Colors.BLUE_FG}CFD mining example completed!{Colors.RESET}\n")


if __name__ == '__main__':
    main()
