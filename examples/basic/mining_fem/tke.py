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


def mine_tke(sequence, episodes_num, window_size, threads=1):
    algo = desbordante.fem.TKE()
    algo.load_data(sequence=sequence)
    algo.execute(episodes_num=episodes_num, window_size=window_size, threads=threads)
    return algo.get_top_k_frequent_episodes()


banner("Discovering the Top-K Frequent Episodes (TKE)")

printlns(
    "The AFEM example (examples/basic/mining_fem/afem.py) showed that AFEM "
    "requires a minsup threshold: too low a value produces an "
    "unmanageably large result and a slower search, too high a value omits "
    "relevant episodes, and there is no way to determine an appropriate "
    "value without already knowing the data. TKE finds the k most frequent "
    "episodes instead, without requiring minsup: it takes the number k of "
    "episodes to return directly."
)
prints("This example follows:")
print()
print("    P. Fournier-Viger, Y. Yang, P. Yang, J. C.-W. Lin, U. Yun. TKE:")
print("    Mining Top-K Frequent Episodes. IEA/AIE 2020, pp. 832-845.")
print()
printlns(
    "Read examples/basic/mining_fem/afem.py first if you have not already: "
    "it introduces the terms this example builds on."
)

print(f"{CYAN}Key definitions{RESET}")
print("-" * 80)

print("  * top-k frequent           given a complex event sequence, a")
print("    episode mining           window_size and an integer k > 0, find a set T of k")
print("                             episodes such that their support is greater than or")
print("                             equal to that of any episode not in T. If several")
print("                             episodes are tied on the cutoff support value, more")
print("                             than one such set exists; TKE breaks ties by")
print("                             discovery order (see the note on threads below).")
print()

print(f"{YELLOW}>>> A note on determinism.{RESET}")
prints(
    "TKE is not a randomized algorithm: support counting is exact, as in "
    "AFEM and MaxFEM. However, its search runs in parallel, and thread "
    "scheduling can affect the result in two ways. First, when several "
    "episodes are tied on the support value at the k-th position, which "
    "one is reported depends on which thread reaches it first. Second, all "
    "three algorithms prune an episode as soon as it appears infrequent "
    "and do not search its super-episodes further, even though a specific "
    "super-episode could still be frequent; which episode gets pruned "
    "before its super-episodes are explored can depend on thread "
    "scheduling. To keep this example's output reproducible, every call "
    "below uses threads=1."
)
print()

print(f"{CYAN}Dataset{RESET}")
print("-" * 80)

printlns(
    "The same event sequence used in examples/basic/mining_fem/afem.py."
)

rows = read_sequence_file(DATASET)
print_sequence_table(rows)

print(f"{CYAN}Algorithm parameters{RESET}")
print("-" * 80)

print("  * sequence     path to a sequence file, or an in-memory Python iterable")
print("                 of (event set, timestamp) pairs - see AFEM example's")
print("                 Scenario 3 (examples/basic/mining_fem/afem.py) for how")
print("                 to build one.")
print()
print("  * episodes_num the number k of top episodes to return. Positive integer,")
print("                 default 10. This replaces minsup entirely.")
print()
print("  * window_size  same meaning as in AFEM/MaxFEM: an occurrence longer than")
print("                 this (in timestamp units) is not counted. Positive integer,")
print("                 default 5.")
print()
print("  * threads      number of worker threads. 0 uses all available CPU cores;")
print("                 see the determinism note above for why this example fixes")
print("                 it to 1. Unlike AFEM/MaxFEM, TKE has no tasks_num_multiplier")
print("                 option.")
print()


print(f"{CYAN}Example{RESET}")
print("-" * 80)

printlns(
    "Let's search the sequence above for its top-3 most frequent episodes."
)

episodes_k3 = mine_tke(DATASET, episodes_num=3, window_size=2)
print("  top-3 episodes (window_size=2):")
print_episodes(episodes_k3)

printlns(
    "examples/basic/mining_fem/afem.py's Scenario 2 obtains the "
    "same three episodes by setting minsup=3; here no support threshold is "
    "specified."
)


banner("See also")

print("Related primitives in Desbordante:")
print("  * All frequent episode mining     -  examples/basic/mining_fem/afem.py")
print("  * Maximal frequent episode mining -  examples/basic/mining_fem/maxfem.py")
print()
