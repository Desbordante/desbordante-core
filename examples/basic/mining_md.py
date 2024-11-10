import desbordante
import pandas

ANIMALS_BEVERAGES = 'examples/datasets/animals_beverages.csv'
CARRIER_MERGER = 'examples/datasets/carrier_merger.csv'

Levenshtein = desbordante.md.column_matches.Levenshtein
Equality = desbordante.md.column_matches.Equality
Custom = desbordante.md.column_matches.Custom


def print_and_discover_mds(algo, column_matches, min_support=None):
    print('Searching for MDs...')
    if min_support is None:
        algo.execute(column_matches=column_matches)
    else:
        algo.execute(column_matches=column_matches, min_support=min_support)
    mds = algo.get_mds()
    print('Found MDs:')
    print(*(f'{i} {md}' for i, md in enumerate(mds)), sep='\n')
    return mds


def symbol_jaccard(str1, str2):
    symbols1 = set(str1)
    symbols2 = set(str2)
    intersection_size = len(symbols1 & symbols2)
    union_size = len(symbols1) + len(symbols2) - intersection_size
    return intersection_size / union_size


def animals_beverages():
    print('First, the animals_beverages dataset will be inspected.')
    table = pandas.read_csv(ANIMALS_BEVERAGES)
    print(table)
    print()

    algo = desbordante.md.algorithms.HyMD()
    algo.load_data(left_table=(ANIMALS_BEVERAGES, ',', True))

    print('In this example, we are going to compare values of every column to '
          'itself using normalized Levenshtein distance.')
    column_matches = [Levenshtein(i, i) for i in range(len(table.columns))]

    mds = print_and_discover_mds(algo, column_matches)

    print('These MDs can also be displayed in short form, showing only non-zero'
          ' decision boundaries:')
    print(*map(lambda md: md.to_short_string(), mds), sep='\n')
    print()


def carrier_merger():
    print("Let's now look at the carrier_merger dataset, obtained as a result "
          "of merger of data from two aircraft carriers (ac1 and ac2).")
    df = pandas.read_csv(CARRIER_MERGER)
    print(df.to_string(index=False))
    print()

    algo = desbordante.md.algorithms.HyMD()
    algo.load_data(left_table=(CARRIER_MERGER, ',', True))
    # equivalent: algo.load_data(left_table=df)

    max_distance = max(df['Distance (km)'])

    print('Now we are going to define the comparisons:')
    print('1) IDs and sources are considered similar if they are equal.')
    print('2) Departure ("From" column) and arrival ("To" column) city names are '
          'going to be compared to themselves ("From" to "From", "To" to "To") '
          'and to each other ("To" to "From", "From" to "To") using the Jaccard'
          ' metric.')
    print('3) Distances are going to be compared to each other using normalized '
          'difference: 1 - |dist1 - dist2| / max_distance.')
    column_matches = [
        Equality('id', 'id'),
        Equality('Source', 'Source'),
        Custom(symbol_jaccard, 'From', 'From', symmetrical=True, equality_is_max=True,
               measure_name='jaccard'),
        Custom(symbol_jaccard, 'To', 'To', symmetrical=True, equality_is_max=True,
               measure_name='jaccard'),
        Custom(symbol_jaccard, 'To', 'From', symmetrical=True, equality_is_max=True,
               measure_name='jaccard'),
        Custom(symbol_jaccard, 'From', 'To', symmetrical=True, equality_is_max=True,
               measure_name='jaccard'),
        Custom(lambda d1, d2: 1 - abs(int(d1) - int(d2)) / max_distance, 'Distance (km)',
               'Distance (km)', symmetrical=True, equality_is_max=True,
               measure_name='normalized_distance')]

    print_and_discover_mds(algo, column_matches)
    print('It is clear to see that ID determines every other attribute, but '
          'there are no dependencies that indicate that.')
    print('In the same manner, one would expect names of departure and arrival '
          'cities being similar to indicate distances also being similar. There '
          'is indeed a dependency like that, which is dependency 2. That '
          'dependency matches the "To" and "From" values to themselves. However,'
          ' it also makes sense for there to be a dependency that matches a "To"'
          ' value to a "From" value or the other way around.')
    print('And yet, none of these dependencies are presented in the answer. '
          'This is because they do not satisfy an interestingness criterion: '
          'their support is too low.')
    print('"Support" in this case means the number of record pairs with similar'
          ' values.')
    print("By default, when there is only one source table, the minimum support"
          " is set to one greater than its number of records. As their support "
          "is lower than that, these dependencies are pruned.")
    print()

    print("Let's decrease the minimum support to 6.")
    print_and_discover_mds(algo, column_matches, 6)
    print('Now these dependencies are present, they are the first five of the '
          'ones displayed.')
    print('However, there also several dependencies that "do not make sense", '
          'like "the departure city and closeness in distance determines the '
          'arrival city". These only hold because the dataset being inspected'
          ' does not happen to contain a counterexample.')
    print()

    print('We can also increase the minimum support requirement. This can help '
          'us find the dependencies that are more reliable, with more examples '
          'supporting them.')
    print_and_discover_mds(algo, column_matches, round(len(df) * 1.5))


def main():
    print('In this example we are discovering MDs as defined in "Efficient '
          'Discovery of Matching Dependencies" by Schirmer et al. Initially, we'
          ' define columns the values of which are going to be compared and the'
          ' measure according to which similarity of values is going to be '
          'determined. The HyMD algorithm then finds the set of decision '
          'boundaries of all MDs that are enough to infer MDs that satisfy some'
          ' requirements (interestingness criteria) and hold on the data.')
    animals_beverages()
    print('-' * 80)
    print()
    carrier_merger()


if __name__ == '__main__':
    main()
