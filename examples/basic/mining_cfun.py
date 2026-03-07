"""CFD mining example with visualization using Desbordante algorithms."""

from collections import defaultdict

import desbordante
import numpy as np
import pandas

TABLE_PATH = 'examples/datasets/play_tennis.csv'
MINIMUM_SUPPORT = 3

if __name__ == '__main__':
    tableDF = pandas.read_csv(TABLE_PATH)
    algo = desbordante.cfd.algorithms.CFUN()
    algo.load_data(table=tableDF)
    algo.execute(cfd_minsup=MINIMUM_SUPPORT)
    result = algo.get_cfds()
    print("options: \nMINIMUM SUPPORT =", MINIMUM_SUPPORT)
    print("displaying the first five (or fewer) discovered CFDs:\n")
    for cfd in result[:5]:
        print(cfd)
