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
    """Parses the "events|timestamp" text format used by Desbordante sequence
    datasets. Only used here to pretty-print the dataset; the algorithm reads
    the file itself via load_data."""
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


def mine(sequence, minsup, window_size):
    algo = desbordante.fem.AFEM()
    algo.load_data(sequence=sequence)
    algo.execute(minsup=minsup, window_size=window_size)
    return algo.get_frequent_episodes()


banner("Discovering All Frequent Episodes (AFEM)")

printlns(
    "AFEM (All Frequent Episode Miner) finds every episode - every "
    "subsequence of events - that occurs often enough in a sequence of "
    "timestamped events."
)

print(f"{YELLOW}>>> Definition 1 (Complex event sequence) [1].{RESET}")
prints(
    "A complex event sequence is a time-ordered list of (event set, "
    "timestamp) pairs. An event is an identifying number: its meaning is "
    "irrelevant, only its time of occurrence and recurrence matter. Several "
    "events occurring at the same timestamp form a simultaneous event set."
)
print()

print(f"{YELLOW}>>> Definition 2 (Episode) [1, 2].{RESET}")
prints(
    "An episode is an ordered list of event sets, written as "
    "X1 -> X2 -> ... -> Xp. An episode with a single event set is a "
    "parallel episode (its events must all fire together); an episode where "
    "every event set has just one event is a serial episode (its events "
    "fire one after another). Desbordante works with the general case, a "
    "composite episode, which combines both."
)
print()

print(f"{YELLOW}>>> Definition 3 (Occurrence and support) [2].{RESET}")
prints(
    "An occurrence of an episode is a time interval [ts, te] in which the "
    "episode's event sets appear, in order, at increasing timestamps. Only "
    "occurrences shorter than a user-chosen window_size count. The support "
    "of an episode is the number of distinct start points ts among its "
    "occurrences (called head frequency). An episode is frequent once its "
    "support reaches minsup."
)
print()

print(f"{CYAN}Dataset{RESET}")
print("-" * 80)

printlns(
    "Four kinds of events, numbered 1 to 4, occur over 11 timestamps below; "
    "events 1 and 2 occasionally fire together."
)

rows = read_sequence_file(DATASET)
print_sequence_table(rows)

print(f"{CYAN}Algorithm parameters{RESET}")
print("-" * 80)

print("  * sequence              path to a sequence file, or an in-memory Python")
print("                          iterable of (event set, timestamp) pairs (see")
print("                          Scenario 3 below).")
print()
print("  * minsup                minimum support an episode must reach to be")
print("                          reported. Positive integer, default 1.")
print()
print("  * window_size           an occurrence longer than this (in timestamp")
print("                          units) is not counted. Positive integer, default 5.")
print()
print("  * threads               number of worker threads for the composite-episode")
print("                          search. 0 (default) uses all available CPU cores.")
print()
print("  * tasks_num_multiplier  ratio of parallel tasks to threads (default 3.0),")
print("                          a scheduling setting that only affects speed.")
print()


banner("Scenario 1. Mining with minsup=2, window_size=2")

episodes = mine(DATASET, minsup=2, window_size=2)
print(f"Found {len(episodes)} frequent episode(s):")
print()
print_episodes(episodes)


banner("Scenario 2. Raising minsup to 3")

printlns(
    "Raising minsup from 2 to 3 removes less frequent episodes from the "
    "result: only three of the seven episodes from Scenario 1 satisfy this "
    "stricter condition."
)

episodes_strict = mine(DATASET, minsup=3, window_size=2)
print(f"  found {len(episodes_strict)} frequent episode(s):")
print_episodes(episodes_strict)

printlns(
    "minsup must be chosen without prior knowledge of the data, so there is "
    "no way to determine an appropriate value in advance. "
    "examples/basic/mining_tke.py's TKE avoids this: instead of minsup, it "
    "takes the number of episodes to return as a parameter."
)


banner("Scenario 3. Loading the same data as a Python iterable")

printlns(
    "The sequence parameter does not have to be a file path: Desbordante "
    "also accepts any Python iterable of (event set, timestamp) pairs, "
    "which is useful when the events are already available in memory, for "
    "example after being read from a database."
)

in_memory_events = [(set(events), ts) for ts, events in rows]

episodes_in_memory = mine(in_memory_events, minsup=2, window_size=2)
matches = sorted(episodes_in_memory) == sorted(episodes)
print(f"  found {len(episodes_in_memory)} frequent episode(s), matching Scenario 1: {matches}")
print()

printlns(
    "Passing the same 8 event sets as Python tuples instead of a file "
    "produces an identical result."
)


banner("See also")

print("Related primitives in Desbordante:")
print("  * Maximal frequent episode mining -  examples/basic/mining_maxfem.py")
print("  * Top-k frequent episode mining    -  examples/basic/mining_tke.py")
print()

print("References:")
print("  [1] H. Mannila, H. Toivonen, A. I. Verkamo. Discovery of Frequent")
print("      Episodes in Event Sequences. Data Mining and Knowledge Discovery")
print("      1(3), 259-289, 1997.")
print("  [2] P. Fournier-Viger, M. S. Nawaz, Y. He, Y. Wu, F. Nouioua, U. Yun.")
print("      MaxFEM: Mining Maximal Frequent Episodes in Complex Event")
print("      Sequences. MIWAI 2022, pp. 86-98.")
print()
