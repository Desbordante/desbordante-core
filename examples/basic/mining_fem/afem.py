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
printlns(
    "AFEM (and the other FEM algorithms) mine sequential data."
)
prints(
    "Frequent episode mining problem along with the core definitions and "
    "concepts was introduced in:"
)
print()
print("    H. Mannila, H. Toivonen, A. I. Verkamo. Discovery of Frequent")
print("    Episodes in Event Sequences. Data Mining and Knowledge Discovery")
print("    1(3), 259-289, 1997.")
print()
prints("The AFEM algorithm itself was introduced later, in:")
print()
print("    P. Fournier-Viger, M. S. Nawaz, Y. He, Y. Wu, F. Nouioua, U. Yun.")
print("    MaxFEM: Mining Maximal Frequent Episodes in Complex Event")
print("    Sequences. MIWAI 2022, pp. 86-98.")
print()

print(f"{CYAN}Key definitions{RESET}")
print("-" * 80)

print("  * event set                a subset of E = {1, 2, ..., m}, the finite set of")
print("                             events. The events in one event set are assumed to")
print("                             fire together, so it is also called a simultaneous")
print("                             event set.")
print()
print("  * complex event sequence   a time-ordered list of (event set, timestamp)")
print("                             pairs S = <(SE_t1, t1), (SE_t2, t2), ..., (SE_tn, tn)>.")
print()
print("  * episode                  an ordered list of event sets X1 -> X2 -> ... -> Xp,")
print("                             where Xi appears before Xj for i < j. An episode with")
print("                             a single event set is a parallel episode (p=1); one")
print("                             where every event set is a single event is a serial")
print("                             episode (|Xi|=1); the general case is also called")
print("                             a composite episode.")
print()
print("  * occurrence               a time interval [ts, te] such that the episode's")
print("                             event sets X1, ..., Xp appear, in order, at")
print("                             increasing timestamps within [ts, te]. occSet(episode)")
print("                             is the set of occurrences shorter than window_size.")
print()
print("  * support                  the number of distinct start points ts among an")
print("                             episode's occurrences: sup(episode) = |{ts : [ts, te]")
print("                             in occSet(episode)}| (also called head frequency).")
print()
print("  * frequent episode mining  given a complex event sequence, a threshold")
print("                             minsup > 0 and a window_size > 0, enumerate all")
print("                             episodes with sup(episode) >= minsup.")
print()

print(f"{CYAN}Dataset{RESET}")
print("-" * 80)

printlns(
    "Four kinds of events, numbered 1 to 4, occur over 11 timestamps "
    "below."
)

printlns(
    "The whole stream below is a single complex event sequence. {1, 2} at "
    "t=3 and t=7 is a parallel event set: it occurs, in full, at exactly "
    "those two timestamps, so the parallel episode {1, 2} has support 2. "
    "Event 1 alone fires at five different timestamps (1, 2, 3, 6, 7), so "
    "the single-event episode 1 has support 5. Chaining that single event "
    "into the parallel pair gives 1 -> {1, 2}, a composite episode. "
    "Its occurrences are: {[2, 3], [6, 7]}."
)

rows = read_sequence_file(DATASET)
print_sequence_table(rows)

print(f"{CYAN}Dataset format{RESET}")
print("-" * 80)

printlns(
    "examples/datasets/fem/episodes_1.txt uses the simple SPMF-style "
    "sequence format all FEM datasets in Desbordante share: one line per "
    "timestamp, listing its event set as ascending, space-separated, "
    "duplicate-free event ids, then a '|', then the timestamp itself - both "
    "non-negative integers, e.g. '1 2|7' means events 1 and 2 fire at t=7. "
    "The '|timestamp' suffix may be omitted for every line at once, in "
    "which case Desbordante numbers the lines 0, 1, 2, ... itself; a single "
    "file cannot mix both styles. Across the whole file, timestamps must be "
    "strictly increasing."
)

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
print("                          a scheduling setting that only affects speed. Must")
print("                          be positive; roughly 1-8 is meaningful - much")
print("                          lower leaves threads idle near the end of the")
print("                          search, much higher creates so many tiny tasks")
print("                          that scheduling overhead dominates.")
print()


banner("Scenario 1. Mining example")

printlns("Let's mine the event stream above with parameters minsup=2, window_size=2.")

episodes = mine(DATASET, minsup=2, window_size=2)
print(f"Found {len(episodes)} frequent episode(s):")
print()
print_episodes(episodes)

printlns(
    "Reading the list: '->' chains event sets into successive, later-in-time "
    "steps (e.g. 1 -> 2 means event 1 occurs, then later event 2); braces "
    "group events that must fire together into one parallel event set (e.g. "
    "{1, 2}). 1 -> {1, 2} chains a single-event step into a parallel "
    "one and is therefore a composite episode."
)


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
    "examples/basic/mining_fem/tke.py's TKE algorithm avoids this: instead "
    "of minsup, it takes the number of episodes to return as a parameter."
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
    "produces an identical result. examples/basic/mining_fem/maxfem.py "
    "and examples/basic/mining_fem/tke.py accept sequences the same "
    "way; refer back here for how to build the in-memory iterable."
)


banner("See also")

print("Related primitives in Desbordante:")
print("  * Maximal frequent episode mining -  examples/basic/mining_fem/maxfem.py")
print("  * Top-k frequent episode mining   -  examples/basic/mining_fem/tke.py")
print()
