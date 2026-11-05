import desbordante

DATASET = "examples/datasets/hue/utility_sequence.txt"
MIN_UTILITY = 0.7
MAX_DURATION = 3

algo = desbordante.huespan.algorithms.Huespan()
algo.load_data(utility_sequence=DATASET)
algo.execute(min_utility = MIN_UTILITY, max_duration = MAX_DURATION)
episodes = algo.get_high_utility_episodes()

print(f"HUE-Span found {len(episodes)} high utility episode(s)")
print("High utility episodes:")
for ep in episodes:
    print(f"  utility = {ep.utility}, events = {ep.events}")
