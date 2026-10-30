"""Mine exact and approximate denial constraints with HybridDC."""

import desbordante

TABLE = 'examples/datasets/taxes_1.csv'

algo = desbordante.dc.algorithms.HybridDC()
algo.load_data(table=(TABLE, ',', True))

# A DC forbids a conjunction of predicates for every pair of distinct rows.
# Zero allows no violating pairs. HybridDC uses exact evidence inversion here.
# A positive threshold selects approximate inversion and bounds the fraction
# of violating ordered row pairs, measured by the g1 error metric.
for threshold in (0.0, 0.1):
    algo.execute(evidence_threshold=threshold, shard_length=0, threads=2)
    print(f'Denial constraints with at most {threshold:.0%} violating row pairs:')
    for dc in sorted(str(dc) for dc in algo.get_dcs()):
        print(dc)
    print()
