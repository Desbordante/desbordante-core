import desbordante

DATASET = 'examples/datasets/erminer/SIGN.txt'

algo = desbordante.erminer.algorithms.ERMiner()

algo.run_algorithm(
    min_support=0.7,
    min_confidence=0.75,
    input_file=DATASET,
    output_file='erminer_output.txt'
)

result = algo.get_rules()

print(f'Found {len(result)} sequential rules '
      f'at minsup = 0.7, minconf = 0.75:\n')

print('Sequential rules:')
for rule in result:
    print(f'{rule.antecedent} -> {rule.consequent}  '
          f'(support: {rule.support}, '
          f'confidence: {rule.confidence:.3f})')

algo.print_stats()