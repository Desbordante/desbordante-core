import desbordante
import pandas
from colorama import Fore, Style, Back

TABLE = 'examples/datasets/dog_breeds.csv'

DOWN_ARROW = "      |\n      |\n      V"

def print_rule_part(rule_part, columns):
    for column_index, value in rule_part.items():
        print(f'{columns[column_index]}{value}')


def print_10_nars(nars, df_columns):
    for i, nar in enumerate(nars[:10], start=1):
        print(f"NAR {i}:{Style.BRIGHT}")
        print_rule_part(nar.ante, df_columns)   
        print(DOWN_ARROW)
        print_rule_part(nar.cons, df_columns)
        print(f"   support = {nar.support}")
        print(f"   confidence = {nar.confidence}")
        print(f"{Style.RESET_ALL}")


if __name__ == '__main__':
    algo = desbordante.nar.algorithms.DES()
    algo.load_data(table=(TABLE, ',', True))
    print("Numerical Association Rules (NAR) are an extension of traditional "
          "Association Rules (AR), which help to discover patterns in data. Unlike ARs, "
          "which work with binary attributes (e.g., whether an item was purchased "
          "or not), NARs can handle numerical data (e.g., how many units of an item "
          "were purchased). This makes NARs more flexible for discovering relationships "
          "in datasets with numerical data.")
    print("Suppose we have a table containing students' exam grades and how many "
          "hours they studied for the exam. Such a table might hold the following "
          "numerical association rule:\n")
    print(f"{Style.BRIGHT}Study_Hours[15.5 - 30.2] {Fore.BLUE}⎤-Antecedent")
    print(f"{Fore.RESET}Subject[Topology]        {Fore.BLUE}⎦")
    print(f"{Fore.RESET}{DOWN_ARROW}")
    print(f"Grade[3 - 5]             {Fore.BLUE}]-Consequent")
    print(f"{Fore.RESET}   support = 0.21")
    print("   confidence = 0.93")
    print()
    print(f"{Style.RESET_ALL}This rule states that students who study Topology for "
          "between 15.5 and 30.2 hours will receive a grade between 3 and 5. This "
          "rule has support of 0.21, which means that 21% of rows in the dataset "
          "satisfy both the antecedent's and consequent's requirements. This rule also "
          "has confidence of 0.93, meaning that 93% of rows that satisfy the "
          "antecedent also satisfy the consequent. Note that attributes can be integers, "
          "floating point numbers, or strings.\n")
    print('Desbordante implements an algorithm called "Differential Evolution Solver" '
          '(DES), described by Iztok Fister et al. in "uARMSolver: A framework for '
          'Association Rule Mining". It is a nature-inspired stochastic optimization '
          "algorithm.\n")
    print("As a demonstration of working with some of DES' parameters, let's inspect "
          "a dataset containing information about 159 dog breeds.\n")
    df = pandas.read_csv(TABLE)

    print("Fragment of the dog_breeds.csv table:")
    print(df)
    print("\nA fragment of the table is presented above. In total, each dog breed has"
          " 13 attributes.")
    print("Now, let's mine some NARs. We will use a minimum support of 0.1 and a minimum "
          "confidence of 0.7. We will also use a population size of 500 and "
          "max_fitness_evaluations of 700. Larger values for max_fitness_evaluations "
          "tend to return larger rules encompassing more attributes. The population size "
          "parameter affects the number of NARs being generated and mutated. Larger values "
          "are slower but output more NARs.\n")
    algo.execute(minconf=0.7, minsup=0.1, population_size=500,
                 max_fitness_evaluations=700)
    if len(algo.get_nars()) != 1:
        raise ValueError("example requires that a single NAR was mined")
    discovered_nar = algo.get_nars()[0]
    print_10_nars([discovered_nar], df.columns)
    
    print("\nThe above NAR is the only one discovered with these settings. The NAR "
          "states that about 92% of all dog breeds of type "
          "'Hound' have an intelligence rating between 6 and 8 out of 10 and are between "
          "sizes 0 and 4 out of 5 (0 being 'Toy' and 5 being 'Giant'). This suggests "
          "that, in general, hounds are intelligent dogs and no more than "
          "8% of all hounds are of 'Giant' size. Let's see if that is true.\n")

    hound_rows = df[df['Type'] == 'Hound']
    
    violating_row_indices = []
    min_intelligence = discovered_nar.cons[9].lower_bound
    max_intelligence = discovered_nar.cons[9].upper_bound
    for i, (_, row) in enumerate(hound_rows.iterrows()):
        intelligence = row['Intelligence']
        if intelligence < min_intelligence or intelligence > max_intelligence:
            violating_row_indices.append(i)
        
    header, *hound_row_strings = hound_rows[['Name', 'Type', 'Intelligence', 'Size']].to_string().splitlines()
    for i, hound_row_string in enumerate(hound_row_strings):
        if i in violating_row_indices:
            print(f"{Back.RED}{hound_row_string}{Back.RESET}")
        else:
            print(hound_row_string)
    
    print("\nAs observed, only 2 rows with 'Type' equal to 'Hound' fall outside "
          "the intelligence rating range of 6 to 8. These two records account for "
          "the (27-2)/27 ~= 92% confidence level of this rule.\n")
    print("Let's try again, but this time with different settings. This time, minimum support "
          "will have a more lenient value of 0.05 and the population size will be 700. "
          "This will help discover more NARs. The value of max_fitness_evaluations "
          "will also need to be increased to 1500 in accordance with the population "
          "size to produce a non-empty result.\n")
    algo.execute(minconf=0.7, minsup=0.05, population_size=700,
                 max_fitness_evaluations=1500)
    print_10_nars(algo.get_nars(), df.columns)
