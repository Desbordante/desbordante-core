import desbordante
import pandas as pd
pyro = desbordante.Pyro()
df = pd.read_csv('examples/datasets/iris.csv', sep=',', header=0)
pyro.load_data(df)
pyro.execute(error=0.0)
pyro.get_fds()
pyro.execute(error=0.1)
pyro.get_fds()
pyro.execute(error=0.2)
pyro.get_fds()
pyro.execute(error=0.3)
pyro.get_fds()
