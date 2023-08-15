from inspect import getmembers, isclass

import desbordante as desb

for algorithm_name, type_ in getmembers(desb, isclass):
    if not (issubclass(type_, desb.FdAlgorithm)
            and type_ is not desb.FdAlgorithm):
        continue
    algorithm = type_()
    for option_name in algorithm.get_possible_options():
        print(option_name, algorithm.get_option_type(option_name))
    algorithm.load_data('WDC_satellites.csv', ',', False)
    algorithm.execute()
    print(algorithm_name, algorithm.get_fds())
