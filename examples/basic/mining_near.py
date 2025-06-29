import desbordante

TABLE_TABULAR = 'examples/datasets/rules_book_rows.csv'
TABLE_SINGULAR = 'examples/datasets/rules_book.csv'

if __name__ == '__main__':
    algo = desbordante.near.algorithms.Default()
    algo.load_data(table=(TABLE_TABULAR, ',', False), input_format='tabular')
    algo.execute()
    print(algo.get_nears())