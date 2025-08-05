from typing import Dict, List, Tuple
from collections import defaultdict
import matplotlib.pyplot as plt
import desbordante as db
import networkx as nx
import pandas as pd


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
        # TODO: Consider using max heap to speed up things
        max_key = max(self.graph, key=lambda x: len(self.graph[x]))
        for neighbor in self.graph[max_key]:
            self.graph[neighbor].remove(max_key)

        del self.graph[max_key]
        self.nodes.remove(max_key)
        self.removed_nodes.append(max_key)

    # Check if the graph contains any edges
    def __has_edges(self) -> bool:
        return any(self.graph[node] for node in self.graph)

    # Remove the highest degree node while graph has edges
    def clean(self) -> None:
        while self.__has_edges():
            self.__remove_highest_degree_node()

    def draw(self, title: str, is_blocked: bool = True) -> None:
        plt.figure()
        plt.title(title)
        G = nx.Graph()
        G.add_nodes_from(self.nodes)
        for node, neighbours in self.graph.items():
            [G.add_edge(node, neighbour) for neighbour in neighbours]
        nx.draw(G, with_labels=True)
        plt.show(block=is_blocked)

def print_table(table: str) -> None:
    data = pd.read_csv(table, header=None)
    data.index = data.index + 1
    headers = data.iloc[0]
    data = data[1:]
    data.columns = headers
    print(data, end="\n\n")

def main():
    print("This is an advanced example explaining how to use Denial Constraint (DC) verification for data cleaning.\n"
    "A basic example of using Denial Constraints is located in examples/basic/verifying_dc.py.\n")

    print("DC verification is perfomed by the Rapidash algorithm:\n"
    "Zifan Liu, Shaleen Deep, Anna Fariha, Fotis Psallidas, Ashish Tiwari, and Avrilia\n"
    "Floratou. 2023. Rapidash: Efficient Constraint Discovery via Rapid Verification.\n"
    "URL: https://arxiv.org/abs/2309.12436\n")

    print("DC φ is a conjunction of predicates of the following form:\n"
    "∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m)\n\n"
    "DCs involve comparisons between pairs of rows within a dataset.\n"
    "A typical DC example, derived from a Functional Dependency such as A -> B,\n"
    "is expressed as: \"∀s, t ∈ R, s ≠ t, ¬(t.A == s.A ∧ t.B ≠ s.B).\"\n"
    "This denotes that for any pair of rows in the relation, it should not be the case\n"
    "that while the values in column A are equal, the values in column B are unequal.\n\n"
    "Consider the following dataset:\n")

    TABLE_1 = 'examples/datasets/taxes_3.csv'
    SEPARATOR = ','
    HAS_HEADER = True
    DC = "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate)"
    DO_COLLECT_VIOLATIONS = True

    # Load data
    verifier = db.dc_verification.algorithms.Default()
    verifier.load_data(table=(TABLE_1, SEPARATOR, HAS_HEADER))

    print_table(TABLE_1)

    print("And the following Denial Constraint:\n"
    "!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate).\n"
    'We use "and" instead of "∧" and "¬" instead of "!" for easier representation.\n\n'
    "The constraint tells us that for all people in the same state,\n"
    "a person with a higher salary must have a higher tax rate.\n"
    "Now, we run the algorithm in order to see if the constraint holds.\n")

    # Execute algorithm
    verifier.execute(denial_constraint=DC, do_collect_violations=DO_COLLECT_VIOLATIONS)

    dc_holds = verifier.dc_holds()

    print("DC " + DC + " holds: " + str(dc_holds), end='\n\n')

    violations = verifier.get_violations()
    viol_str = ", ".join(map(str, violations))

    print("Now let's examine why the constraint doesn't hold. To do this,\n"
    "we can get the violations (pairs of rows that contradict the given constraint):\n"
    f"{viol_str}\n\n"
    "As we can see, there are multiple pairs of rows contradicting the given constraint.\n"
    "We can leverage this knowledge to find typos or other mistakes.\n")

    print("Denial constraint is a very powerful tool for detecting various inaccuracies\n"
    "and errors in data. Our implementation allows users to additionally identify violations — pairs\n"
    "of records for which the condition described by the Denial Constraint is not satisfied.\n"
    "If we are confident that a certain DC should hold, we can then construct an algorithm to repair the data.\n\n"

    "We will demonstrate this by creating a proof-of-concept algorithm based on a graph approach.\n"
    "The vertices in our graph will be the records, and the presence of a violation will be represented\n"
    "as an edge between two vertices. Thus, our task can be reformulated as follows: to find the\n"
    "minimum number of vertices that need to be removed in order for the graph to become edgeless.\n"
    "In this process, we remove vertices in such a way that all incident edges are also removed.\n"
    "At the record level, removing a vertex will mean performing an operation to delete or edit\n"
    "the record, such that the edge (violation) no longer exists.\n\n"

    "In the code, this graph functionality is implemented (albeit in a naive way) inside the DataCleaner class.\n")

    print("Close figure windows to continue")

    # Start cleaning algorithm
    cleaner = DataCleaner(violations)
    
    cleaner.draw("Graph before", False)
    cleaner.clean()
    cleaner.draw("Graph after")

    nodes = sorted(cleaner.removed_nodes)
    nodes_str = ", ".join(map(str, nodes))

    print(f"The cleaning algorithm returns the following records: {nodes_str}\n"
    "This means we should consider the following records incorrect and correct them.\n"
    "Indeed, the following records contain typos and should be changed:\n\n"
    "5     NewYork   6000       0.04  -->    NewYork   6000         0.4\n"
    "9   Wisconsin   3000        0.9  -->  Wisconsin   3000        0.09\n"
    "13      Texas   5000       0.05  -->      Texas   5000         0.5\n\n")

    TABLE_2 = 'examples/datasets/taxes_4.csv'
    SEPARATOR = ','
    HAS_HEADER = True

    verifier = db.dc_verification.algorithms.Default()
    verifier.load_data(table=(TABLE_2, SEPARATOR, HAS_HEADER))

    verifier.execute(denial_constraint=DC, do_collect_violations=DO_COLLECT_VIOLATIONS)
    dc_holds = verifier.dc_holds()

    print("The dataset after repairs:\n")
    print_table(TABLE_2)

    print("Now we can check if the constraint holds by running the algorithm again.\n\n"
    f"DC " + DC + " holds: " + str(dc_holds), end='\n\n'
    "After fixing the typos in the initial dataset, the constraint holds.\n")


if __name__ == "__main__":
    main()
