import desbordante as db

# Parameters for data_stats.load_data(...)
DATASET_PATH = "examples/datasets/Workshop.csv"
SEPARATOR = ','
HAS_HEADER = True


def main() -> None:
    data_stats = db.statistics.algorithms.Default()
    data_stats.load_data(table=(DATASET_PATH, SEPARATOR, HAS_HEADER))
    data_stats.execute()

    # Some of column and table methods
    column_methods = {
        "Avg": data_stats.get_average,
        "Sum of squares": data_stats.get_sum_of_squares,
        "Median": data_stats.get_median,
        "Min": data_stats.get_min,
        "Max": data_stats.get_max,
        "Distinct": data_stats.get_number_of_distinct,
        "Corrected std": data_stats.get_corrected_std,
        "Min chars in a row": data_stats.get_min_number_of_chars,
        "Max chars in a row": data_stats.get_max_number_of_chars,
        "Min words in a row": data_stats.get_min_number_of_words,
        "Max words in a row": data_stats.get_max_number_of_words,
        "Char vocabulary": data_stats.get_vocab,
        "Word vocabulary": data_stats.get_words
    }

    table_methods = {
        "Columns with null": data_stats.get_columns_with_null,
        "Columns with all unique values": data_stats.get_columns_with_all_unique_values,
        "Number of columns": data_stats.get_number_of_columns
    }

    num_cols = data_stats.get_number_of_columns()

    # Stats for a whole table
    for description, method in table_methods.items():
        res = method()
        if type(res) is set:
            res = sorted(res)
        if res is not None:
            print(f"{description}: {res}")
    print()

    # Stats for each column
    for i in range(num_cols):
        print(f"Column num: {i}")
        for description, method in column_methods.items():
            res = method(i)
            if type(res) is set:
                res = sorted(res)
            if res is not None:
                print(f"{description}: {res}")
        print()

    # Print all statistics at once
    print(data_stats.get_all_statistics_as_string())


if __name__ == "__main__":
    main()
