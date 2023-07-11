from inspect import getmembers, isclass

import desbordante as desb

for name, type_ in getmembers(desb, isclass):
    if not (issubclass(type_, desb.FdAlgorithm)
            and type_ is not desb.FdAlgorithm):
        continue
    algorithm = type_()
    algorithm.load_data('WDC_satellites.csv', ',', False)
    algorithm.execute()
    print(name, algorithm.get_fds())
