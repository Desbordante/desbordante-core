"""CFD mining example with visualization using Desbordante algorithms."""

from collections import defaultdict

import desbordante
import numpy as np
import pandas

TABLE_PATH = 'examples/datasets/play_tennis.csv'
MINIMUM_SUPPORT = 3
GREEN_BG_CODE = '\033[1;42m'
GREEN_FG_CODE = '\033[1;32m'
RED_BG_CODE = '\033[1;41m'
DEFAULT_BG_CODE = '\033[1;49m'
DEFAULT_FG_CODE = '\033[1;37m/'


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
