import desbordante
import pandas as pd
from operator import attrgetter

GREEN_CODE = "\033[1;42m"
RED_CODE = "\033[1;41m"
BLUE_CODE = "\033[1;46m"
DEFAULT_COLOR_CODE = "\033[1;49m"

def print_results(verifier, data, parameter):
    if verifier.mfd_holds():
        print(GREEN_CODE, "MFD holds", DEFAULT_COLOR_CODE)
    else:
        highlights_list = verifier.get_highlights()
        print(RED_CODE, "MFD does not hold due to the following items:",
              DEFAULT_COLOR_CODE)
        for index, cluster in enumerate(highlights_list, start=1):
            print(f"{BLUE_CODE} #{index} cluster: {DEFAULT_COLOR_CODE}")
            highlighted_line_indices = list(map(attrgetter("data_index"),
                                                cluster))
            print(data.iloc[highlighted_line_indices])
            max_distance = max(map(attrgetter("max_distance"), cluster))
            print(f"Max distance: {max_distance} > {parameter}")
        print()

def check_mfd(lhs_indices, rhs_indices, metric, parameters,
              table, mfd_text, metric_algorithm = None):
    data = pd.read_csv(table, header=[0])
    print(data)

    algo = desbordante.mfd_verification.algorithms.Default()
    algo.load_data(table=(table, ",", True))
    for mfd_parameter in parameters:
        algo.execute(
            lhs_indices=lhs_indices,
            metric=metric,
            parameter=mfd_parameter,
            rhs_indices=rhs_indices,
            metric_algorithm=metric_algorithm
        )
        print(f"Checking whether {mfd_text} MFD holds with "
              f"parameter δ = {mfd_parameter}")
        print_results(algo, data, mfd_parameter)

def mfd_coordinates():
    METRIC = "euclidean"
    METRIC_ALGORITHM = "calipers"
    LHS_INDICES = [1]
    RHS_INDICES = [2,3]
    PARAMETERS = [1, 0.1, 0.01, 0.001, 0.0001]
    TABLE="examples/datasets/addresses_coordinates.csv"

    print("To do that, let's explore addresses_coordinates.csv and try "
          "verifying a metric functional dependency in it.\n")

    check_mfd(LHS_INDICES, RHS_INDICES, METRIC, PARAMETERS, TABLE,
              "[Address] -> [Latitude, Longitude]", METRIC_ALGORITHM)

    print("As we can notice from the results, decreasing δ "
          "tightens the constraint, and, as such, yields more "
          "violations for smaller values. Namely, 0.001 is the first "
          "value we tried that resulted in the MFD no longer holding. The "
          "algorithm also provides us with record pairs that prevent the "
          "MFD from holding, with those records being grouped into "
          f"{BLUE_CODE}clusters{DEFAULT_COLOR_CODE}.\n")
    print("Let's take a closer look at them.\n")
    print("Both google and geocoder provided coordinates for multiple "
          "addresses that didn't have matching coordinates across different "
          "sources, yet were close enough for us to assume that they point to "
          "approximately the same place with a degree of accuracy "
          "that is sufficient for us to consider them basically the same. "
          f"For example, in {BLUE_CODE}Cluster 3{DEFAULT_COLOR_CODE}, "
          "the appartments differ by 0.000229 in longitude and by 0.00004 "
          "in latitude, with the 2 points being merely "
          "around 0.01979 km (or 0.012298 miles) apart, "
          "which is considered to be the same place with "
          "parameter δ = 0.001, but violates the MFD with "
          "parameter δ = 0.0001.")
    print("-" * 80)

def mfd_basic():
    METRIC = "euclidean"
    LHS_INDICES = [0]
    RHS_INDICES = [2]
    PARAMETERS = [5, 3]
    TABLE="examples/datasets/theatres_mfd.csv"

    print(f"{BLUE_CODE}Metric Functional Dependency{DEFAULT_COLOR_CODE} "
          "(MFD) is a type of relaxed functional "
          "dependency designed to account for small deviations that would "
          "otherwise invalidate regular functional dependencies. Those "
          "deviations are measured using an arbitrary metric defined by "
          "the user, "
          "making this definition applicable in a variety of situations.\n")
    print("Semi-formal definition for those interested:")
    print("Given a parameter δ and metric Δ, an MFD is defined to hold if "
          "Δ(x, y) <= δ holds for all x and y, where x and y are different "
          "right-hand side attribute values of records sharing the "
          "same left-hand side attribute values.\n")
    print("Let's start by investigating theatres_mfd.csv and "
          "trying to verify a metric functional dependency in it.\n")

    check_mfd(LHS_INDICES, RHS_INDICES, METRIC, PARAMETERS, TABLE,
              "[Movie] -> [Duration]")

    print("We learnt that in this case, the specified MFD does not hold, and "
          "there are two clusters of rows that contain values that prevent "
          f"our MFD from holding. A {BLUE_CODE}cluster{DEFAULT_COLOR_CODE} "
          "(with respect to a fixed FD) is a collection of rows that share "
          "the same left-hand side, but differ in the right-hand side.")
    print("Let's take a closer look at the indicated clusters.\n")
    print('"Don Quixote" has been found with 3 different durations: 135, 139 '
          "and 140. From this, we can conclude that "
          "(assuming euclidian distance), the maximum distance between "
          "values is 140 - 135 = 5, which exceeds the provided "
          "parameter of δ = 3. When we tried to verify the same MFD on this "
          "dataset with δ = 5, this cluster adhered to the MFD as 5 <= δ, "
          "and the whole MFD held.")
    print('"Romeo and Juliet" had the exact same issue: the biggest '
          "difference between durations of different versions appeared to "
          "have been 165 - 160 = 5, which meant that the MFD held with "
          "the greater parameter value of δ = 5.")
    print("-" * 80)

def mfd_strings():
    METRIC = "cosine"
    METRIC_ALGORITHM = "brute"
    LHS_INDICES = [1]
    RHS_INDICES = [2]
    PARAMETERS = [0.75, 0.5, 0.25, 0.1, 0.01]
    TABLE="examples/datasets/addresses_names.csv"
    bigrams_table="examples/datasets/bigrams.csv"

    print("Let's showcase this by checking addresses_names.csv and "
          "trying to verify a metric functional dependency in it.\n")

    check_mfd(LHS_INDICES, RHS_INDICES, METRIC, PARAMETERS, TABLE,
              "[SSN] -> [Address]", METRIC_ALGORITHM)

    print("To get into the intricacies of how the distance has been "
          "calculated here, we need to first define what is known as an "
          f"n-gram. {BLUE_CODE}N-grams{DEFAULT_COLOR_CODE} are collections "
          "of adjacent symbols of fixed "
          'length, such as "aab" or "ba". Depending on the length, they are '
          "called bigrams (2), trigrams (3) and so on.\n")
    print("For the purposes of illustration, we picked the RHS values from "
          "the cluster violating the MFD with δ = 0.01 but not δ = 0.1. "
          "The following table displays the number of occurrences of every "
          "bigram (columns) occurring in any of these values (rows). "
          "In total, there are 30 bigrams across two strings.")

    bigrams = pd.read_csv(bigrams_table, header=[0])
    print(bigrams)

    print("Let's interpret the rows in the table as coordinates of "
          "two vectors. We have ways to compare them. For example, "
          "we can assess how similar they are by calculating "
          "S(A, B) = A*B / (||A||*||B||), where A*B is a dot product, and "
          "||A|| is a magnitude of vector A. This is called "
          '"cosine similarity". To quantify how different two strings '
          "are, we'll use the metric "
          'Δ(x, y) = 1 - S(x, y), also known as "cosine distance".')
    print("In this example, a parameter of δ = 0.1 is sufficient "
          "for the algorithm to not consider "
          '"New York Av., Washington, D.C." and '
          '"New York Ave., Washington, D.C." to be different addresses.')

print(DEFAULT_COLOR_CODE)

mfd_basic()
print("MFDs are not limited to metrics of one attribute. Let's take a look "
      "at an example that compares distance between "
      f"{GREEN_CODE}coordinates{DEFAULT_COLOR_CODE} of addresses.")
mfd_coordinates()
print("MFD discovery can even be performed on "
      f"{GREEN_CODE}strings{DEFAULT_COLOR_CODE} using cosine distance.")
mfd_strings()
