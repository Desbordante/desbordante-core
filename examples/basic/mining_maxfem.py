import desbordante
import textwrap

YELLOW = "\033[1;33m"
CYAN = "\033[1;36m"
RESET = "\033[0m"

DATASET = "examples/datasets/fem/episodes_1.txt"


def prints(s):
    print(textwrap.fill(s, 80))


def printlns(s):
    prints(s)
    print()


def banner(title):
    print("=" * 80)
    print(f"{CYAN}{title}{RESET}")
    print("=" * 80)


def read_sequence_file(path):
    rows = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            events_part, _, ts_part = line.partition("|")
            rows.append((int(ts_part), [int(e) for e in events_part.split()]))
    return rows


def fmt_events(events):
    return ", ".join(str(e) for e in sorted(events))


def print_sequence_table(rows):
    print(f"  {'t':>3} | event set")
    print(f"  {'-' * 3}-+-{'-' * 30}")
    for ts, events in rows:
        print(f"  {ts:>3} | {{{fmt_events(events)}}}")
    print()


def fmt_episode(structure, support):
    parts = [("{" + fmt_events(event_set) + "}") if len(event_set) > 1
             else fmt_events(event_set) for event_set in structure]
    return " -> ".join(parts) + f"   (support: {support})"


def print_episodes(episodes):
    ordered = sorted(episodes, key=lambda ep: (-ep[1], len(ep[0]), ep[0]))
    for i, (structure, support) in enumerate(ordered, start=1):
        print(f"  #{i:<2} {fmt_episode(structure, support)}")
    print()


def mine_afem(sequence, minsup, window_size):
    algo = desbordante.fem.AFEM()
    algo.load_data(sequence=sequence)
    algo.execute(minsup=minsup, window_size=window_size)
    return algo.get_frequent_episodes()


def mine_maxfem(sequence, minsup, window_size, threads=0):
    algo = desbordante.fem.MaxFEM()
    algo.load_data(sequence=sequence)
    algo.execute(minsup=minsup, window_size=window_size, threads=threads)
    return algo.get_max_frequent_episodes()


def is_included(sub_structure, sup_structure):
    i = 0
    for event_set in sup_structure:
        if i < len(sub_structure) and set(sub_structure[i]).issubset(set(event_set)):
            i += 1
    return i == len(sub_structure)


banner("Discovering Maximal Frequent Episodes (MaxFEM)")

printlns(
    "examples/basic/mining_afem.py showed that AFEM reports every frequent "
    "episode, including many that are sub-episodes of longer ones also "
    "present in the output. MaxFEM [1] reports only the frequent episodes "
    "that are not contained in any other frequent episode - the maximal "
    "frequent episodes - of which there are typically far fewer."
)

print(f"{YELLOW}>>> Definition 1 (Strict inclusion) [1].{RESET}")
prints(
    "An episode alpha = <Y1, ..., Yi> is strictly included in an episode "
    "beta = <X1, ..., Xp> (written alpha [ ] beta) if Y1, ..., Yi embed, in "
    "order, into some subsequence of X1, ..., Xp - each Yj is a subset of "
    "the matching Xk - and beta has strictly more events overall. For "
    "example, 1 [ ] 1 -> 2, because the single event set {1} embeds into "
    "the first step of the longer episode."
)
print()

print(f"{YELLOW}>>> Definition 2 (Maximal frequent episode) [1].{RESET}")
prints(
    "A frequent episode is maximal if no other frequent episode strictly "
    "includes it. MaxFEM reports the set of all maximal frequent episodes."
)
print()

print(f"{CYAN}Dataset{RESET}")
print("-" * 80)

printlns(
    "The same event sequence used in examples/basic/mining_afem.py."
)

rows = read_sequence_file(DATASET)
print_sequence_table(rows)

print(f"{CYAN}Algorithm parameters{RESET}")
print("-" * 80)

prints(
    "MaxFEM takes the same four parameters as AFEM: minsup, window_size, "
    "threads and tasks_num_multiplier (see examples/basic/mining_afem.py "
    "for details on each)."
)
print()


banner("Scenario 1. AFEM vs. MaxFEM, side by side (minsup=2, window_size=2)")

afem_episodes = mine_afem(DATASET, minsup=2, window_size=2)
maxfem_episodes = mine_maxfem(DATASET, minsup=2, window_size=2)

print(f"  AFEM found {len(afem_episodes)} frequent episode(s):")
print_episodes(afem_episodes)

print(f"  MaxFEM found {len(maxfem_episodes)} maximal frequent episode(s):")
print_episodes(maxfem_episodes)

printlns(
    "Five of AFEM's seven episodes - event 1, event 1 repeated, the "
    "parallel pair {1, 2}, and two chains built on event 1 - are "
    "sub-episodes of the chain 1 -> {1, 2}. MaxFEM keeps only that chain "
    "and event 3, which is not contained in any other frequent episode. "
    "These two episodes are the maximal front for minsup=2."
)


banner("Scenario 2. Checking Definition 2 against Scenario 1")

printlns(
    "Definition 2 implies that every frequent episode is a sub-episode of "
    "some maximal one. This is verified below against Scenario 1's output: "
    "each of AFEM's 7 episodes should be included in one of MaxFEM's 2 "
    "maximal episodes."
)

ordered_afem = sorted(afem_episodes, key=lambda ep: (-ep[1], len(ep[0]), ep[0]))
for structure, support in ordered_afem:
    maximal = next(m for m in maxfem_episodes if is_included(structure, m[0]))
    print(f"  {fmt_episode(structure, support):<32} included in {fmt_episode(*maximal)}")
print()

printlns(
    "Each of the 7 episodes is included in one of the 2 maximal episodes, "
    "as Definition 2 requires. The maximal front is a lossless summary of "
    "the full set of frequent episodes."
)


banner("See also")

print("Related primitives in Desbordante:")
print("  * All frequent episode mining -  examples/basic/mining_afem.py")
print("  * Top-k frequent episode mining -  examples/basic/mining_tke.py")
print()

print("References:")
print("  [1] P. Fournier-Viger, M. S. Nawaz, Y. He, Y. Wu, F. Nouioua, U. Yun.")
print("      MaxFEM: Mining Maximal Frequent Episodes in Complex Event")
print("      Sequences. MIWAI 2022, pp. 86-98.")
print()
