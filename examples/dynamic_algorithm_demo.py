import desbordante
import pandas

TABLE = 'examples/datasets/pfd.csv'

algo = desbordante.dynamic.algorithms.Default(table=(TABLE, ',', True))

def print_result(algo):
    result = algo.get_result()
    res = [(int((i.split(',')[0].split(' ')[1])), i) for i in result]
    res.sort()
    for item in res:
        print('   ', item[1])
    ins_, del_ = algo.get_result_diff()
    if len(ins_):
        print('  Added into result:')
        for item in ins_:
            print('   ', item)
    if len(del_):
        print('  Removed from result:')
        for item in del_:
            print('   ', item)

print('Init result:')
print_result(algo)

df1 = pandas.DataFrame({'X': [100, 101], 'Y': [100, 101]})
algo.process(insert=df1)
print('Insert test result:')
print_result(algo)

algo.process(delete=[3, 4, 12])
print('Delete test result:')
print_result(algo)

df2 = pandas.DataFrame({'X': [4], 'Y': [6]})
algo.process(update_old=[6], update_new=df2)
print('Update test result:')
print_result(algo)

df3 = pandas.DataFrame({'X': [100, 100, 102], 'Y': [200, 200, 202]})
df4 = pandas.DataFrame({'X': [102, 105], 'Y': [202, 205]})
algo.process(insert=df3, update_new=df4, delete=[1, 2, 13], update_old=[5, 7])
print('Mixed test result:')
print_result(algo)
