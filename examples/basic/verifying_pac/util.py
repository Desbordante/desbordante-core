import csv

from tabulate import tabulate

RED = '\033[31m'
YELLOW = '\033[33m'
BOLD_YELLOW = '\033[1;33m'
GREEN = '\033[32m'
BLUE = '\033[34m'
CYAN = '\033[36m'
BOLD = '\033[1;37m'
ENDC = '\033[0m'


def csv_to_str(filename: str) -> str:
    with open(filename, newline='') as table:
        rows = list(csv.reader(table, delimiter=','))
    headers = rows[0]
    rows = rows[1:]
    return tabulate(rows, headers=headers)
