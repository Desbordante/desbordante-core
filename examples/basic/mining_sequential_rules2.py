#!/usr/bin/env python3
import desbordante
from pathlib import Path

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def colored(text, color):
    return f"{color}{text}{bcolors.ENDC}"

DATASET_SMALL = 'examples/datasets/erminer/sequences_small.txt'
DATASET_BIG = 'examples/datasets/erminer/sequences_big.txt'
MINSUP = 0.5
MINCONF = 0.7

INTRO_TEXT = (
    "ERMiner (Efficient Rule Miner) is an algorithm for mining sequential rules.\n\n"
    "A sequential rule X -> Y means that if a sequence contains all items from X,\n"
    "then it will contain all items from Y later in the sequence.\n\n"
    "Reference: Fournier-Viger, P. et al. (2014). ERMiner: Sequential Rule Mining\n"
    "using Equivalence Classes. PAKDD 2014.\n\n"

)

PARAM_INFO = (
    "Algorithm parameters:\n"
    "- min_support: Minimum support threshold (0.0-1.0).\n"
    "  Support = number of sequences containing the rule / total sequences.\n\n"
    "- min_confidence: Minimum confidence threshold (0.0-1.0).\n"
    "  Confidence = support(X ∪ Y) / support(X).\n\n"
    "- max_antecedent_size: Maximum size of left part (default: unlimited).\n"
    "- max_consequent_size: Maximum size of right part (default: unlimited).\n"
)

DATASET_INFO = """
Dataset format (SPMF format):
Each sequence ends with -2, itemsets separated by -1.
Example: "1 2 -1 3 -1 -2" means:
  Itemset1: {1,2}
  Itemset2: {3}
  
Sample dataset (shopping sequences):
Customer1: 1 2 -1 3 -1 -2    (bought milk, bread then eggs)
Customer2: 1 -1 2 -1 3 -1 -2 (bought milk then bread then eggs)
Customer3: 2 -1 1 -1 3 -1 -2 (bought bread then milk then eggs)
Customer4: 1 2 -1 4 -1 -2    (bought milk, bread then cheese)
"""

def print_section(title, content):
    print(colored(f"\n{'='*60}", bcolors.HEADER))
    print(colored(f" {title}", bcolors.HEADER))
    print(colored(f"{'='*60}", bcolors.HEADER))
    print(content)

def analyze_rules(rules, title):
    print(colored(f"\n{title}:", bcolors.OKCYAN))
    print(f"Total rules found: {colored(len(rules), bcolors.BOLD)}")
    
    if not rules:
        return
    
    rules_by_size = {}
    for rule in rules:
        size = len(rule.antecedent)
        if size not in rules_by_size:
            rules_by_size[size] = []
        rules_by_size[size].append(rule)
    
    print("\nRules by antecedent size:")
    for size in sorted(rules_by_size.keys()):
        print(f"  Size {size}: {len(rules_by_size[size])} rules")
    
    print(colored("\nSample rules:", bcolors.OKGREEN))
    for i, rule in enumerate(rules[:5]):
        print(f"\nRule #{i+1}:")
        print(f"  If [{', '.join(map(str, rule.antecedent))}]")
        print(f"  Then [{', '.join(map(str, rule.consequent))}]")
        print(f"  Support: {rule.support} sequences, Confidence: {rule.confidence:.3f}")

def demo_basic_run():
    print_section("1. Basic ERMiner Run", 
                 "Running ERMiner with default parameters on small dataset")
    
    algo = desbordante.erminer.algorithms.algorithms.ERMiner()
    
    print(colored(f"\nInput file: {DATASET_SMALL}", bcolors.OKBLUE))
    print(f"min_support = {MINSUP}, min_confidence = {MINCONF}")
    
    algo.run_algorithm(
        min_support=MINSUP,
        min_confidence=MINCONF,
        input_file=DATASET_SMALL,
        output_file='erminer_output.txt'
    )
    
    rules = algo.get_rules()
    analyze_rules(rules, "Basic Run Results")
    return rules

def demo_parameter_impact():
    print_section("2. Parameter Impact", 
                 "How changing parameters affects the results")
    
    algo = desbordante.erminer.ERMiner()
    
    test_params = [
        (0.3, 0.5, "Low support, low confidence"),
        (0.5, 0.7, "Medium support, medium confidence"),
        (0.7, 0.9, "High support, high confidence")
    ]
    
    for minsup, minconf, desc in test_params:
        print(colored(f"\n{desc}:", bcolors.OKCYAN))
        print(f"  minsup={minsup}, minconf={minconf}")
        
        algo.run_algorithm(minsup, minconf, DATASET_SMALL, 'temp.txt')
        rules = algo.get_rules()
        print(f"  Rules found: {len(rules)}")
        
        if rules:
            best_rule = max(rules, key=lambda r: r.confidence)
            print(f"  Best rule: {best_rule}")

def demo_size_constraints():
    print_section("3. Size Constraints", 
                 "Limiting antecedent and consequent sizes")
    
    algo = desbordante.erminer.ERMiner()
    
    print(colored("\nWithout size limits:", bcolors.OKCYAN))
    algo.run_algorithm(0.3, 0.5, DATASET_SMALL, 'temp.txt')
    rules_no_limit = algo.get_rules()
    print(f"Rules found: {len(rules_no_limit)}")
    
    print(colored("\nWith max_antecedent_size = 1:", bcolors.OKCYAN))
    algo.set_max_antecedent_size(1)
    algo.run_algorithm(0.3, 0.5, DATASET_SMALL, 'temp.txt')
    rules_limit_ant = algo.get_rules()
    print(f"Rules found: {len(rules_limit_ant)}")
    
    if rules_limit_ant:
        print("All rules have antecedent size 1:")
        for rule in rules_limit_ant[:3]:
            print(f"  {rule}")

def demo_error_detection():
    print_section("4. Error Detection with ERMiner",
                 "Using sequential rules to find data inconsistencies")
    
    # Создаем синтетический датасет с аномалией
    print(colored("\nAnalyzing customer purchase patterns...", bcolors.OKBLUE))
    
    # Нормальные данные: после покупки телефона (1) покупают чехол (2)
    # Аномалия: один клиент купил чехол без телефона
    
    anomaly_dataset = """1 -1 2 -1 -2
1 -1 2 -1 -2
1 -1 2 -1 -2
2 -1 -2  # Аномалия!
1 -1 2 -1 -2
"""
    with open('temp_anomaly.txt', 'w') as f:
        f.write(anomaly_dataset)
    
    algo = desbordante.erminer.algorithms.ERMiner()
    algo.run_algorithm(0.6, 0.8, 'temp_anomaly.txt', 'temp_out.txt')
    
    rules = algo.get_rules()
    print(colored("\nFound rules:", bcolors.OKGREEN))
    for rule in rules:
        print(f"  {rule}")
        if rule.antecedent == [2] and rule.consequent == [1]:
            print(colored("  This rule suggests that buying case (2) leads to phone (1)",
                         bcolors.WARNING))
            print("    But in our data, one customer bought case without phone!")
    
    Path('temp_anomaly.txt').unlink(missing_ok=True)

def demo_performance_scaling():
    """Демонстрация производительности на разных данных"""
    print_section("5. Performance Scaling", 
                 "How algorithm performs on larger datasets")
    
    if not Path(DATASET_BIG).exists():
        print(colored(f"\nBig dataset not found: {DATASET_BIG}", bcolors.WARNING))
        return
    
    import time
    
    algo = desbordante.erminer.algorithms.ERMiner()
    
    for dataset, name in [(DATASET_SMALL, "Small"), (DATASET_BIG, "Big")]:
        print(colored(f"\n{name} dataset:", bcolors.OKCYAN))
        
        start = time.time()
        algo.run_algorithm(0.3, 0.5, dataset, 'temp.txt')
        elapsed = time.time() - start
        rules = algo.get_rules()
        
        print(f"  Rules found: {len(rules)}")
        print(f"  Time: {elapsed:.2f} seconds")
        print(f"  Output file: {algo.get_rules()} rules written")

def main():
    print(colored("="*60, bcolors.HEADER))
    print(colored(" ERMiner: Sequential Rule Mining Examples", bcolors.HEADER))
    print(colored("="*60, bcolors.HEADER))
    
    print_section("Introduction", INTRO_TEXT)
    print_section("Parameters", PARAM_INFO)
    print_section("Dataset Format", DATASET_INFO)
    
    demo_basic_run()
    demo_parameter_impact()
    demo_size_constraints()
    demo_error_detection()
    
    if Path(DATASET_BIG).exists():
        demo_performance_scaling()
    
    print_section("Conclusion", 
                 "ERMiner successfully discovered sequential patterns!\n"
                 "Try changing parameters to see different results.\n")    
    Path('erminer_output.txt').unlink(missing_ok=True)
    Path('temp.txt').unlink(missing_ok=True)
    Path('temp_out.txt').unlink(missing_ok=True)

if __name__ == "__main__":
    main()