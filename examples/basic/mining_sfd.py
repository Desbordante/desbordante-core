import desbordante
import pandas as pd
import numpy as np
from tabulate import tabulate


def mine_sfd():
    algo = desbordante.sfd.algorithms.Default()
    algo.load_data(table=df)
    algo.execute(min_sfd_strength=MIN_SFD_STRENGTH_MEASURE,
                 delta=DELTA,
                 max_false_positive_probability=MAX_FALSE_POSITIVE_PROBABILITY,
                 only_sfd=ONLY_SFD,
                 min_cardinality=MIN_CARDINALITY,
                 max_amount_of_categories=MAX_AMOUNT_OF_CATEGORIES,
                 min_skew_threshold=MIN_SKEW_THRESHOLD,
                 min_structural_zeroes_amount=MIN_STRUCTURAL_ZEROES_AMOUNT,
                 max_different_values_proportion=MAX_DIFF_VALS_PROPORTION)
    fds = algo.get_fds()
    cors = algo.get_correlations()
    if len(fds):
        print("Soft functional dependencies:")
        for fd in fds:
            print(fd)
    else:
        print("No sfd")
    if len(cors):
        print("Correlations:")
        for cor in cors:
            print(cor)
    else:
        print("No correlations")


if __name__ == '__main__':
    print('A soft functional dependency (SFD) is yet another type of relaxed functional dependency (FD),\n'
          'a kind of FD that tolerates some degree of errors in data. It has the form of X->Y, where X\n'
          'and Y are single attributes of a table. SFDs were first introduced by Ihab Ilyas et al. in\n'
          '"CORDS:  automatic discovery of correlations and soft functional dependencies". They are\n'
          'also known as the approximate FDs (AFD) with the \\rho metric in \n"Measuring Approximate'
          'Functional Dependencies: a Comparative Study" by Marcel Parciak et al.\n\n'
          'Using CORDS algorithm to discover SFDs is pretty similar to using plain FD discovery\n'
          'algorithms, which is discussed in mining_fd.py.\n\n'
          'Therefore, in this example we will try to construct and describe a dataset on which discovery\n'
          'of SFDs is meaningful.\n')

    n_rows = 10000  # Number of rows in the dataset
    print("First of all let us construct a synthetic dataset consisting of %d rows and 5 columns.\n" % n_rows)
    np.random.seed(65)  # Set the random seed for reproducibility

    # Generate attributes
    A = np.random.randint(1, 100, n_rows)  # Initial column
    B = 2 * A + np.random.choice([0, 1], n_rows, p=[0.95, 0.05])
    C = A + B + np.random.choice([0, 27], n_rows, p=[0.85, 0.15])
    D = 3 * C + np.random.choice([0, 5, 17, 34], n_rows,
                                 p=[0.6, 0.1, 0.1, 0.2])
    E = np.random.randint(1, 3, n_rows)  # Independent column
    # Create a DataFrame
    df = pd.DataFrame({
        'A': A,
        'B': B,
        'C': C,
        'D': D,
        'E': E
    })
    print("Our dataset contains 5 columns: A, B, C, D, E")
    print("A is a column of random integers belonging to [1,100).\n"
          "B is generated as A * 2 with 5% chance of deviation by 1\n"
          "C is generated as A + B with 15% chance of deviation by 27\n"
          "D is generated as 3 * C with 10% chance of deviation by 5, 10% chance of deviation by 17, 20% chance of "
          "deviation by 34\n"
          "E is a column of random integers belonging to [1,2]\n\n"
          "Here are the first 10 rows of our dataset:")
    print(tabulate(df, headers=['A', 'B', 'C', 'D', 'E'], tablefmt='psql', showindex=True)[:560])
    print(
        "As you can see our dataset is constructed in such a way that exact FDs 'almost hold'\n"
        "meaning there are some tuples which violate them.\n\n"
        "We expect the following SFDs to hold:\n"
        "[A] -> B\n"
        "[A] -> C\n"
        "[A] -> D\n"
        "[B] -> C\n"
        "[B] -> D\n"
        "[C] -> D\n")

    ONLY_SFD = False
    MIN_CARDINALITY = 0.1
    MAX_DIFF_VALS_PROPORTION = 0.99
    MIN_SFD_STRENGTH_MEASURE = 0.1
    MIN_SKEW_THRESHOLD = 0.5
    MIN_STRUCTURAL_ZEROES_AMOUNT = 3e-01
    MAX_FALSE_POSITIVE_PROBABILITY = 1e-06
    DELTA = 0.11
    MAX_AMOUNT_OF_CATEGORIES = 100

    print("The core parameters of the CORDS algorithm are:\n"
          "only_sfd: a boolean flag indicating whether we want to mine correlations besides SFDs or not\n\n"

          "min_cardinality: (1 - min_cardinality) * n_rows denotes the minimum amount of distinct values "
          "in a column to be considered a soft key\n\n"

          "min_sfd_strength: (1 - min_sfd_strength) denotes the minimum strength threshold of SFD in\n"
          "order to be included into the result\n\n"

          "max_false_positive_probability: (1 - max_false_positive_probability) denotes maximum\n"
          "acceptable probability of a false-positive correlation test result \n\n"

          "max_amount_of_categories: denotes the maximum amount of allowed categories for the chi-squared test\n"
          )

    print(
        "There are other parameters besides listed above. For more detailed descriptions of them we\n"
        "recommend you to look into the original paper and /src/core/config/descriptions.h\n\n"
        "Due to the random nature of the algorithm we might have to run it a few times to get the full picture:\n")

    for i in range(1, 11):
        print('-' * 15)
        print("Iteration:", i)
        mine_sfd()
    print('-' * 15, '\n')
    print(
        "As you can see, on some iterations our expected soft functional dependencies are mined as correlations.\n"
        "But if we relax our SFD measure threshold to 0.7 (set min_sfd_strength to 0.3) we will see the expected\n"
        "output.\n")
    MIN_SFD_STRENGTH_MEASURE = 0.3

    for i in range(1, 11):
        print('-' * 15)
        print("Iteration:", i)
        mine_sfd()
    print('-' * 15)
print(
    "Also you may notice that left hand sides (LHS) and right hand sides of our SFDs are reversed. This\n"
    "happens because the algorithm considers column with higher cardinality to be the LHS.")
