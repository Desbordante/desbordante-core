import desbordante
import pandas

TABLE = 'examples/datasets/ind_datasets/teaches.csv'

algo = desbordante.dynamic.algorithms.Default(table=(TABLE, ',', True))

result = algo.get_result()
print('First result:')
if result:
    for item in result:
        print(item)
else:
    print('Result is empty')

df1 = pandas.DataFrame({'Instructor ID': ['in1234'], 'Course ID': ['A-1'], 'Year': [1], 'Semester': ['Fall']})
algo.process(insert=df1)
result = algo.get_result()
print('Second result:')
for item in result:
    print(item)
