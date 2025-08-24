import desbordante
import pandas as pd
pyro = desbordante.afd.algorithms.Pyro()  # same as desbordante.afd.algorithms.Default()
df = pd.read_csv('examples/datasets/iris.csv', sep=',', header=None)
pyro.load_data(table=df)
pyro.execute(error=0.0)
print(f'[{", ".join(map(str, pyro.get_fds()))}]')
# [[0 1 2] -> 4, [0 2 3] -> 4, [0 1 3] -> 4, [1 2 3] -> 4]
pyro.execute(error=0.1)
print(f'[{", ".join(map(str, pyro.get_fds()))}]')
# [[2] -> 0, [2] -> 3, [2] -> 1, [0] -> 2, [3] -> 0, [0] -> 3, [0] -> 1, [1] -> 3, [1] -> 0, [3] -> 2, [3] -> 1, [1] -> 2, [2] -> 4, [3] -> 4, [0] -> 4, [1] -> 4]
pyro.execute(error=0.2)
print(f'[{", ".join(map(str, pyro.get_fds()))}]')
# [[2] -> 0, [0] -> 2, [3] -> 2, [1] -> 2, [2] -> 4, [3] -> 4, [0] -> 4, [1] -> 4, [3] -> 0, [1] -> 0, [2] -> 3, [2] -> 1, [0] -> 3, [0] -> 1, [1] -> 3, [3] -> 1]
pyro.execute(error=0.3)
print(f'[{", ".join(map(str, pyro.get_fds()))}]')
# [[2] -> 1, [0] -> 2, [2] -> 0, [2] -> 3, [0] -> 1, [3] -> 2, [3] -> 1, [1] -> 2, [3] -> 0, [0] -> 3, [4] -> 1, [1] -> 0, [1] -> 3, [4] -> 2, [4] -> 3, [2] -> 4, [3] -> 4, [0] -> 4, [1] -> 4]
