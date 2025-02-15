from os import scandir
from sys import argv
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from json import load as j_load

# Read serialized results from JSON file
def read_results(filename: str) -> dict[str: int]:
    with open(filename, 'r') as file:
        return j_load(file)

def read_all_results(dir: str) -> list[dict[str, int]]:
    all_results = []
    # Files have names like "results-2025-01-01:00:00:00.txt"
    for fd in sorted(scandir(dir), key=lambda x: x.name):
        if fd.is_file():
            all_results.append(read_results(fd.path))
    return all_results

def build_plot(results: list[dict[str, int]], name: str, pages: PdfPages) -> None:
    points = []
    for res in results:
        points.append(res.get(name, 0) / 1000)

    _, ax = plt.subplots()
    ax.stairs(points)
    ax.set_title(name)
    ax.set_ylabel('Time, s')
    pages.savefig()

# First argument is folder with test results
# Second argument is filename to store plots
def main():
    if len(argv) >= 3:
        results = read_all_results(argv[1])
        last_res = results[-1]

        pdf = PdfPages(argv[2])

        for name in last_res:
            build_plot(results, name, pdf)
        pdf.close()
    else:
        print('Please, provide 2 parameters')
        exit(1)

if __name__ == '__main__':
    main()
