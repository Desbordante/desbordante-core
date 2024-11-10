import desbordante
import pandas

CARRIER_MERGER = 'examples/datasets/carrier_merger.csv'
CARRIER3 = 'examples/datasets/carrier3.csv'
CARRIER4 = 'examples/datasets/carrier4.csv'

SUPPORT_MULTIPLIER = 1.7

Equality = desbordante.md.column_matches.Equality
Custom = desbordante.md.column_matches.Custom


def symbol_jaccard(str1, str2):
    symbols1 = set(str1)
    symbols2 = set(str2)
    intersection_size = len(symbols1 & symbols2)
    union_size = len(symbols1) + len(symbols2) - intersection_size
    return intersection_size / union_size


def print_mds(df, column_matches):
    algo = desbordante.md.algorithms.HyMD()
    algo.load_data(left_table=df)
    print('Searching for MDs...')
    algo.execute(column_matches=column_matches, min_support=round(len(df) * SUPPORT_MULTIPLIER))
    print('Found MDs:')
    print(*algo.get_mds(), sep='\n')


def main():
    print('In this example we find a meaningful MD and try to use it to enforce'
          ' data integrity.')
    print('We are going to use a dataset of flights between cities.')
    df = pandas.read_csv(CARRIER_MERGER)
    print(df.to_string(index=False))
    print()

    algo = desbordante.md.algorithms.HyMD()
    algo.load_data(left_table=(CARRIER_MERGER, ',', True))

    max_distance = max(df['Distance (km)'])

    print('Now we are going to define the comparisons:')
    print('1) IDs and sources are considered similar if they are equal.')
    print('2) Departure ("From" column) and arrival ("To" column) city names are'
          ' going to be compared to themselves ("From" to "From", "To" to "To") '
          'and to each other ("To" to "From", "From" to "To") using the Jaccard'
          ' metric.')
    print('3) Distances are going to be compared to each other using normalized '
          'difference: 1 - |dist1 - dist2| / max_distance.')
    print(f'{max_distance=}')
    column_matches = [
        Equality('id', 'id'),
        Equality('Source', 'Source'),
        Custom(symbol_jaccard, 'From', 'From', symmetrical=True, equality_is_max=True,
               measure_name='jaccard'),
        Custom(symbol_jaccard, 'To', 'To', symmetrical=True, equality_is_max=True,
               measure_name='jaccard'),
        Custom(lambda d1, d2: 1 - abs(int(d1) - int(d2)) / max_distance, 'Distance (km)',
               'Distance (km)', symmetrical=True, equality_is_max=True,
               measure_name='normalized_distance')]

    print('Searching for MDs...')
    algo.execute(column_matches=column_matches, min_support=round(len(df) * SUPPORT_MULTIPLIER))
    print('Found MDs:')
    print(*algo.get_mds(), sep='\n')

    print("We find a matching dependency. We assume the dependency is "
          "meaningful, i.e. departure and arrival cities can give us a very "
          "accurate estimate of flight distance.")
    print()

    print("Let's test this assumption by adding another carrier's flights and "
          "checking whether the MD still holds.")
    df3 = pandas.read_csv(CARRIER3)
    df = pandas.concat([df, df3], ignore_index=True)
    print(df.to_string(index=False))
    print()

    algo = desbordante.md.algorithms.HyMD()
    algo.load_data(left_table=df)
    print('Searching for MDs...')
    try:
        algo.execute(column_matches=column_matches, min_support=round(len(df) * SUPPORT_MULTIPLIER))
    except ValueError as e:
        print(f'The algorithm threw an exception: {e}')
        print()
    else:
        assert False, "Should error out here..."

    print(f'Inspecting the measures, we find that max_distance ({max_distance})'
          f" is no longer the actual maximum distance. Let's update it.")
    max_distance = max(df['Distance (km)'])
    print(f'{max_distance=}')

    print('Searching for MDs...')
    algo.execute(column_matches=column_matches, min_support=round(len(df) * SUPPORT_MULTIPLIER))
    print('Found MDs:')
    print(*algo.get_mds(), sep='\n')

    print('However, we see that no dependencies were found.')
    print()

    print('By inspecting the dataset, we find that the row for the flight with '
          "ID 18 has an obviously erroneous distance value. Let's fix it.")
    df.at[17, 'Distance (km)'] = 1913
    print(df.to_string(index=False))

    print_mds(df, column_matches)
    print('The dependency is not exactly the same, as we are technically using '
          'a different similarity measure, but it means the same thing as '
          'before.')

    print("Let's add more data from another carrier.")
    df4 = pandas.read_csv(CARRIER4)
    df = pandas.concat([df, df4], ignore_index=True)
    print(df.to_string(index=False))
    print()

    max_distance = max(df['Distance (km)'])
    print(f'This time, we remember to update the maximum distance: '
          f'{max_distance=}')

    print_mds(df, column_matches)
    print('The dependency is no longer present. We inspect the data and find '
          'that "Moscow" and "Saint Petersburg" in the new portion are US '
          'cities, unlike before. We can conclude that our initial assumption '
          'was incorrect, departure and arrival city names are not enough to '
          'determine flight distance accurately.')
    print('To resolve this, let us introduce a new attribute indicating the '
          'region the city is part of.')
    df.insert(2, 'Region', ['non-US'] * 18 + ['US'] * 3)

    print(df.to_string(index=False))

    print('Let us take the new column into account by adding a new column match:')
    print('4) Regions are considered similar if they are equal.')
    column_matches.append(Equality('Region', 'Region'))
    print_mds(df, column_matches)

    print('We have now fixed our data.')
    print('We can see that there are new dependencies in the result. They are '
          'not getting pruned by the support requirement like before, and they '
          'also are not meaningful. With larger data quantities they will '
          'disappear naturally. We may also try increasing the minimum support '
          'threshold, but unfortunately not in this particular case, as the '
          'amount of data is small.')


if __name__ == '__main__':
    main()
