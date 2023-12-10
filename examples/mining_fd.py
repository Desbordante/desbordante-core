import desbordante

TABLE = 'examples/datasets/university_fd.csv'

algo = desbordante.HyFD()
algo.load_data(TABLE, ',', True)
algo.execute()
result = algo.get_fds()
print('FDs:')
for fd in result:
    print(fd)
