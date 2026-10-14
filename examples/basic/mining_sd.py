"""Discover and interpret Sequential Dependency tableaux."""

import math
import textwrap

import desbordante
import pandas as pd


TABLE_PATH = "examples/datasets/sd_mining_poll_times.csv"
LHS_NAME = "PollNum"
RHS_NAME = "Time"
G1 = 9.0
G2 = 11.0
APPROXIMATE_DELTA = 0.2
OUTPUT_WIDTH = 70

CYAN = "\033[1;36m"
YELLOW = "\033[1;33m"
RESET = "\033[0m"


def prints(text):
    print(
        textwrap.fill(
            text,
            width=OUTPUT_WIDTH,
            break_long_words=False,
            break_on_hyphens=False,
        )
    )


def printlns(text):
    prints(text)
    print()


def banner(title):
    print("=" * OUTPUT_WIDTH)
    print(f"{CYAN}{title}{RESET}")
    print("=" * OUTPUT_WIDTH)


def print_parameter(name, description):
    print(f"  * {YELLOW}{name}{RESET}")
    print(
        textwrap.fill(
            description,
            width=OUTPUT_WIDTH,
            initial_indent="    ",
            subsequent_indent="    ",
            break_long_words=False,
            break_on_hyphens=False,
        )
    )


def annotate_gaps(table, g2=G2):
    annotated = table.sort_values(
        [LHS_NAME, RHS_NAME], kind="stable"
    ).reset_index(drop=True)
    gaps = annotated[RHS_NAME].diff()
    upper_bound = "infinity" if g2 < 0 else f"{g2:.0f}"
    annotated["Time gap"] = [
        "-" if pd.isna(gap) else f"{gap:.0f}" for gap in gaps
    ]
    annotated[f"In [{G1:.0f}, {upper_bound}]"] = [
        "-"
        if pd.isna(gap)
        else "yes"
        if G1 <= gap and (g2 < 0 or gap <= g2)
        else "no"
        for gap in gaps
    ]
    return annotated


def print_input_table(table, g2=G2):
    annotated = annotate_gaps(table, g2)
    gap_match_column = annotated.columns[-1]
    column_widths = {
        LHS_NAME: 12,
        RHS_NAME: 12,
        "Time gap": 14,
        gap_match_column: 20,
    }
    print(annotated.to_string(index=False, col_space=column_widths))
    print()


def mine_tableau(table, **options):
    algo = desbordante.sd.algorithms.SDMiner()
    algo.load_data(table=table)
    parameters = {
        "lhs_indices": [0],
        "rhs_indices": [1],
        "g1": G1,
        "g2": G2,
    }
    parameters.update(options)
    algo.execute(**parameters)
    return algo


def print_tableau(algo, name="T"):
    tableau = algo.get_tableau()
    print(f"{YELLOW}Tableau {name}:{RESET}")
    if not tableau:
        print("  No tableau patterns found.")
    else:
        for index, tableau_pattern in enumerate(tableau, start=1):
            print(
                f"  P{index}: {LHS_NAME} in "
                f"[{tableau_pattern.left_x:.0f}, "
                f"{tableau_pattern.right_x:.0f}], "
                f"support = {tableau_pattern.support}, "
                f"confidence = {tableau_pattern.confidence:.3f}"
            )
    print(
        f"Global support({name}) = {algo.get_global_support()} of "
        f"{len(algo.get_sorted_row_indices())} rows"
    )
    print()


def explain_paper():
    banner("1. Paper")
    printlns(
        "This example demonstrates how to mine Sequential Dependencies "
        "(SDs) using the Desbordante library. The algorithm is based on the "
        "article by Lukasz Golab, Howard Karloff, Flip Korn, Avishek Saha, "
        "and Divesh Srivastava. 2009. Sequential dependencies. Proc. VLDB "
        "Endow. 2, 1 (August 2009), 574–585."
    )
    printlns(
        "This example focuses on mining an SD tableau and later compares "
        "the exact and faster strategies. If you want to validate a known "
        "SD, locate its violations, and see how the data can be repaired, "
        "continue with examples/basic/verifying_sd.py."
    )


def explain_objects(table):
    banner("2. Sequential Dependencies, tableau patterns, and tableaux")
    printlns(
        "Imagine that you have measurements that should arrive in a regular "
        "order, but delays and missing records sometimes break that order. "
        "SDMiner helps you find the ranges where the expected sequence is "
        "still visible."
    )

    printlns(
        "We will work with a log of network polls. PollNum is the poll "
        "number, and Time is the time when the poll occurred, in seconds."
    )
    print_input_table(table)
    printlns(
        "To describe the sequence, we first need a column that puts the rows "
        "in order. This column is called X. For our log, X is PollNum, so "
        "poll 1 comes before poll 2. We also need a value to watch as we "
        "move through that order. This column is called Y, and here it is "
        "Time."
    )
    print(f"{YELLOW}SD notation: X -> [g1, g2] Y{RESET}")
    print("  * X tells SDMiner how to order the rows.")
    print("  * Y is the value we want to follow through that order.")
    print("  * g1 is the smallest change in Y that we will accept.")
    print("  * g2 is the largest change in Y that we will accept.")
    printlns("A change equal to either bound is accepted as well.")
    print(
        f"{YELLOW}This example: {LHS_NAME} -> "
        f"[{G1:.0f}, {G2:.0f}] {RHS_NAME}{RESET}"
    )
    printlns(
        "After sorting by PollNum, Time must change by 9 to 11 seconds "
        "between consecutive rows. The change from 10 to 20 is 10 and "
        "satisfies the SD. The next change, from 20 to 25, is 5 and "
        "violates it."
    )
    printlns(
        "At this point, we have chosen the SD ourselves. SDMiner will not "
        "try to guess X, Y, or the gap bounds. Instead, it will look through "
        "the ordered rows and find contiguous ranges where our SD holds "
        "closely enough. Each such range is a tableau pattern. Together, "
        "the returned tableau patterns form one tableau."
    )
    print(f"{YELLOW}Tableau pattern notation:{RESET}")
    print("  P1: PollNum in [1, 2], support = 2, confidence = 1.000")
    print()
    printlns(
        "Suppose SDMiner returns the line above. The bounds [1, 2] tell us "
        "where this tableau pattern starts and ends in X. Its support says "
        "that the range contains 2 rows. Its confidence tells us how well "
        "those rows follow the SD, and confidence 1 means that they follow "
        "it strictly. We call the next tableau patterns P2, P3, and so on. "
        "They all belong to one tableau T, written T = {P1, P2, ...}."
    )
    printlns(
        "Finally, we usually want to know how much of the original data the "
        "whole tableau explains. Global support answers that question by "
        "counting the distinct rows covered by its tableau patterns. If two "
        "tableau patterns overlap, a shared row is counted only once."
    )


def explain_parameters():
    banner("3. SDMiner parameters")
    printlns(
        "Once you have chosen the SD you want to mine — its X and Y columns "
        "and the allowed gap bounds — you can configure SDMiner using the "
        "parameters below. You pass table to load_data() and all other "
        "parameters to execute()."
    )
    print_parameter(
        "table (required)",
        "The data in which SDMiner will search for a tableau. This is the "
        "only parameter passed to load_data()",
    )
    print_parameter(
        "lhs_indices (required)",
        "Determines which column is used as X. Pass a list containing one "
        "zero-based column index. Only one numeric column is currently supported",
    )
    print_parameter(
        "rhs_indices (required)",
        "Determines which column is used as Y. Pass a list containing one "
        "zero-based column index. Only one numeric column is currently supported",
    )
    print_parameter(
        "g1 (required)",
        "Sets the smallest allowed change in Y. It must be finite and "
        "non-negative",
    )
    print_parameter(
        "g2 (required)",
        "Sets the largest allowed change in Y and normally must be at least "
        "g1. A negative finite value removes the upper bound, as Section 6 shows",
    )
    print_parameter(
        "minimum_confidence (default: 1.0)",
        "Controls how closely each tableau pattern must follow the SD. A "
        "value of 1 requires a strict match, while lower values allow more "
        "deviations. You can choose any value from 0 to 1. Section 5 shows "
        "this trade-off",
    )
    print_parameter(
        "minimum_support (default: 1.0)",
        "Controls how much of the data the whole tableau must cover. A value "
        "of 1 requires every row, while lower values allow partial coverage. "
        "You can choose any value from 0 to 1. Section 5 explains how "
        "overlapping coverage is counted",
    )
    print_parameter(
        "interval_strategy (default: 'exact')",
        "SDMiner supports two algorithms for finding candidate intervals for "
        "tableau patterns. "
        "'exact' preserves minimum_confidence. 'approximate' can be faster, "
        "but returned tableau patterns may have lower confidence. "
        "Section 8 compares them",
    )
    print_parameter(
        "assembly_strategy (default: 'exact')",
        "SDMiner supports two algorithms for assembling the tableau. 'exact' "
        "produces the smallest tableau among the candidate intervals found. "
        "'greedy' usually uses less time and memory, "
        "but its tableau may contain more tableau patterns than necessary. "
        "Section 7 compares them",
    )
    print_parameter(
        "delta (default: 0.05)",
        "Only affects the approximate interval strategy. Smaller values keep "
        "the confidence guarantee closer to minimum_confidence but usually "
        "require more work. Section 8 explains the exact guarantee. "
        "Delta must be strictly between 0 and 1",
    )
    print()
    printlns(
        "The defaults give a strict and predictable result, so the first run "
        "only provides the required parameters."
    )


def basic_scenario(table):
    banner("4. First mining run")
    printlns(
        "For our run, we only need to pass the SD that we described "
        "above. We will leave every optional setting at its default. This "
        "asks SDMiner for tableau patterns that follow PollNum -> [9, 11] "
        "Time strictly and for a tableau that covers every row."
    )
    printlns("Here is the input table again before we run the miner.")
    print_input_table(table)
    algo = desbordante.sd.algorithms.SDMiner()
    algo.load_data(table=table)
    algo.execute(
        lhs_indices=[0],
        rhs_indices=[1],
        g1=G1,
        g2=G2,
    )
    print_tableau(algo)
    printlns(
        "These five tableau patterns form one tableau T = {P1, P2, P3, P4, "
        "P5}. P1 contains polls 1 and 2, whose Time gap is 10. P2 contains "
        "polls 3 through 5, whose two gaps are also 10. All three gaps "
        "satisfy the SD."
    )
    printlns(
        "The 15-second and 30-second gaps leave polls 6, 7, and 8 in "
        "separate tableau patterns. P3, P4, and P5 each contain one row, so "
        "there is no consecutive pair that could break the SD. That gives "
        "every tableau pattern confidence 1. Together they cover all 8 "
        "rows, exactly as minimum_support = 1 requires."
    )


def imperfect_scenario(table):
    minimum_confidence = 0.6
    minimum_support = 0.8
    target_support = math.ceil(minimum_support * len(table))

    banner("5. Allowing imperfect tableau patterns")
    printlns(
        "Our strict tableau is quite fragmented because several gaps miss "
        "the interval [9, 11]. Real logs often contain delayed or missing "
        "measurements, so we may prefer a few longer ranges over many small "
        "ones. Imperfect tableau patterns let us make that choice."
    )
    printlns(
        "How does SDMiner decide how much imperfection a range contains? It "
        "uses confidence = (N - OPS) / N. Here N is the tableau pattern's "
        "support. OPS is the smallest number of row insertions and "
        "deletions that would make the SD strict inside that tableau "
        "pattern. These operations only define the score. SDMiner never "
        "changes our input table."
    )
    printlns(
        f"For this run, we lower minimum_confidence to "
        f"{minimum_confidence:.1f}. We also choose minimum_support = "
        f"{minimum_support:.1f}, so the tableau still needs to cover at "
        f"least {target_support} of the {len(table)} rows."
    )
    algo = mine_tableau(
        table,
        minimum_confidence=minimum_confidence,
        minimum_support=minimum_support,
    )
    print_tableau(algo, "T_imperfect")
    printlns(
        "The relaxed confidence gives us two longer tableau patterns. "
        "Let us look at P1 first. It covers polls 1 through 5, whose Time "
        "values are [10, 20, 25, 35, 45]. If we remove the rows with Time "
        "values 10 and 20, we are left with [25, 35, 45]. Both remaining "
        "gaps are 10, so this sequence follows the SD strictly."
    )
    printlns(
        "For P2, removing the rows with Time values 60 and 90 leaves the "
        "same strict sequence. Each tableau pattern contains 5 rows and "
        "needs 2 deletions, giving confidence (5 - 2) / 5 = 0.6."
    )
    printlns(
        "They share polls 3, 4, and 5. Although their individual supports "
        "add up to 10, together they cover 7 distinct rows. This is enough "
        "to meet our 80% target."
    )


def unbounded_gap_scenario(table):
    banner("6. Removing the upper gap bound")
    printlns(
        "Suppose we only care that the time increases by at least 9 seconds. "
        "Long pauses are acceptable, so placing an upper limit on the gap "
        "would not help us. In this situation, we can pass any negative "
        "finite value as g2. We will use g2 = -1 and keep g1 = 9."
    )
    printlns(
        "Here is the log again. The last column now shows which gaps fit "
        "the SD without an upper bound."
    )
    print_input_table(table, g2=-1.0)
    algo = mine_tableau(table, g2=-1.0)
    print_tableau(algo, "T_unbounded")
    printlns(
        "Without an upper bound, the 15-second and 30-second gaps now fit "
        "our SD. The 5-second gap is still too small, so it separates P1 "
        "from P2. These two strict tableau patterns cover the entire log."
    )


def assembly_scenario(table):
    minimum_confidence = 0.75
    minimum_support = 0.8
    target_support = math.ceil(minimum_support * len(table))

    banner("7. Exact and greedy tableau assembly")
    printlns(
        "For this comparison, we return to the original upper bound g2 = 11. "
        "Once SDMiner has found suitable intervals, it still needs to choose "
        "which of them will make up the tableau. If we want the smallest "
        "possible number of tableau patterns, exact assembly is the right "
        "choice. If we care more about speed and memory, greedy assembly is "
        "usually more convenient, although its tableau may be larger."
    )
    printlns(
        f"To compare them fairly, both runs will require confidence "
        f"{minimum_confidence:.2f} and coverage of at least "
        f"{target_support} of {len(table)} rows. We will also use exact "
        "interval generation in both cases. This way, only "
        "assembly_strategy changes."
    )
    exact_algo = mine_tableau(
        table,
        minimum_confidence=minimum_confidence,
        minimum_support=minimum_support,
    )
    print_tableau(exact_algo, "T_exact")
    greedy_algo = mine_tableau(
        table,
        minimum_confidence=minimum_confidence,
        minimum_support=minimum_support,
        assembly_strategy="greedy",
    )
    print_tableau(greedy_algo, "T_greedy")
    printlns(
        "Now we can see the difference that matters to us as users. T_exact "
        "needs three tableau patterns, while T_greedy returns four. Both "
        "cover 7 rows, and every tableau pattern meets the same confidence "
        "threshold. Greedy changes the size of the answer, not the quality "
        "requirements that its tableau patterns must satisfy."
    )
    printlns(
        "The speedup from greedy depends strongly on your data and settings. "
        "In our tests, for example, switching from exact to greedy assembly "
        "with exact interval generation reduced peak process memory about "
        "14-fold, with little change in total mining time. Greedy can save "
        "time as well, but the overall gain depends on the time spent "
        "generating candidates."
    )


def approximate_scenario(table):
    minimum_confidence = 1.0
    delta = APPROXIMATE_DELTA
    relaxed_confidence = minimum_confidence * (1 - delta) / (1 + delta)

    banner("8. Approximate interval generation")
    printlns(
        "On a large table, finding every suitable interval may take more "
        "time than we are willing to spend. Exact interval generation keeps "
        "our requested confidence intact. Approximate generation can finish "
        "sooner, but some returned tableau patterns may have lower "
        "confidence than we requested."
    )
    printlns(
        "This is where delta becomes useful. It tells SDMiner how much "
        "confidence we are prepared to trade for speed. The guarantee is "
        "minimum_confidence * (1 - delta) / (1 + delta). A smaller positive "
        "delta tightens the confidence guarantee, but it "
        "usually asks SDMiner to do more work."
    )
    printlns(
        "Before comparing the results, let us bring the original gaps back "
        "into view."
    )
    print_input_table(table)
    printlns(
        f"For this run, we request confidence "
        f"{minimum_confidence:.1f} and choose delta = {delta:.1f}. That gives "
        f"us a guarantee of about {relaxed_confidence:.3f}. We keep exact "
        "assembly, which lets us focus only on the effect of approximate "
        "interval generation."
    )
    algo = mine_tableau(
        table,
        minimum_confidence=minimum_confidence,
        interval_strategy="approximate",
        assembly_strategy="exact",
        delta=delta,
    )
    print_tableau(algo, "T_approximate")
    printlns(
        "The result now contains four tableau patterns instead of the five "
        "strict ones from Section 4. In particular, P2 joins polls 3 through "
        "6 into one range with confidence 0.750. This is lower than the "
        "1.0 we requested, but it remains above the promised 0.667. That is "
        "the speed and confidence trade-off we asked delta to control."
    )
    printlns(
        "In our tests, switching from exact to approximate generation while "
        "keeping exact assembly made mining about 13 times faster, with "
        "similar memory use. "
        "Switching to greedy assembly then cut total mining time by a "
        "further 13% and reduced peak process memory roughly ninefold."
    )
    printlns(
        "These figures describe the results of our tests, not guaranteed "
        "improvements. On your data, the speedup and memory savings may be "
        "very different, depending on the data and parameter choices. "
        "Approximate generation can even be slower than exact."
    )


def print_related_examples():
    banner("9. See also")
    printlns(
        "In practice, you will often run SDMiner more than once. Start with "
        "the exact defaults, inspect the tableau, and then adjust confidence, "
        "support, or the strategies if the result is too fragmented or the "
        "run takes too long."
    )
    printlns(
        "After discovering a useful tableau, you may want to inspect the SD "
        "from another angle. If you already know the SD and want to check "
        "where it is violated, continue with the verification example. If "
        "you want to discover relationships between orderings themselves, "
        "take a look at the Order Dependency examples."
    )
    print("  * SD verification and violations")
    print("    examples/basic/verifying_sd.py")
    print("  * List-based Order Dependency mining")
    print("    examples/basic/mining_list_od.py")
    print("  * Set-based Order Dependency mining")
    print("    examples/basic/mining_set_od_1.py")


def main():
    table = pd.read_csv(TABLE_PATH)

    explain_paper()
    explain_objects(table)
    explain_parameters()
    basic_scenario(table)
    imperfect_scenario(table)
    unbounded_gap_scenario(table)
    assembly_scenario(table)
    approximate_scenario(table)
    print_related_examples()


if __name__ == "__main__":
    main()
