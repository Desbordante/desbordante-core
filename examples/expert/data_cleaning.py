from typing import Dict, List, Tuple
from collections import defaultdict
import matplotlib.pyplot as plt
import desbordante as db
import networkx as nx
import time


class DataCleaner:
    def __init__(self, violations: List[Tuple[int, int]]) -> None:
        self.graph: Dict[int, List[int]] = defaultdict(list)
        for v1, v2 in violations:
            if v1 != v2:
                self.graph[v1].append(v2)
                self.graph[v2].append(v1)
            else:
                self.graph[v1] = [v1]
        self.nodes: List[int] = list(self.graph.keys())
        self.removed_nodes: List[int] = []

    def __remove_highest_degree_node(self) -> None:
        # TODO: you might want to use max_heap to speed up things
        max_key = max(self.graph, key=lambda x: len(self.graph[x]))
        for neighbor in self.graph[max_key]:
            self.graph[neighbor].remove(max_key)

        del self.graph[max_key]
        self.nodes.remove(max_key)
        self.removed_nodes.append(max_key)

    # Check if the graph contains any edges
    def __has_edges(self) -> bool:
        return any(self.graph[node] for node in self.graph)

    # Remove highest degree node while graph has edges
    def clean(self) -> None:
        print("Cleaning algorithm started")
        while self.__has_edges():
            self.__remove_highest_degree_node()
        print("Cleaning algorithm finished")

    def draw(self, is_blocked: bool = True) -> None:
        plt.figure()
        G = nx.Graph()
        G.add_nodes_from(self.nodes)
        for node, neighbours in self.graph.items():
            [G.add_edge(node, neighbour) for neighbour in neighbours]
        nx.draw(G, with_labels=True)
        plt.show(block=is_blocked)


def main():
    TABLE_1 = 'examples/datasets/taxes_2.csv'
    DC = "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)"
    SEPARATOR = ','
    HAS_HEADER = True

    print("Data loading started")
    verificator = db.dc_verification.algorithms.Default()
    verificator.load_data(table=(TABLE_1, SEPARATOR, HAS_HEADER))
    print("Data loading finished")

    DO_COLLECT_VIOLATIONS = True

    print("Algo execution started")

    verificator.execute(denial_constraint=DC, do_collect_violations=DO_COLLECT_VIOLATIONS)

    print("Algo execution finished")

    dc_holds = verificator.dc_holds()

    print("DC " + DC + " holds: " + str(dc_holds))

    violations = verificator.get_violations()
    cleaner = DataCleaner(violations)

    cleaner.draw(False)
    cleaner.clean()
    cleaner.draw()

    nodes = sorted(cleaner.removed_nodes)
    print(f"Records to be removed: {", ".join(map(str, nodes))}")


if __name__ == "__main__":
    main()
