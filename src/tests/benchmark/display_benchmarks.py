''' Benchmark results visualization tool

This script processes JSON files containing benchmark results and generates a
PDF report with time series plots for each algorithm.

Input:
- Directory with previous JSON results (named 1.json, 2.json, etc.)
- Baseline result JSON file
- Current run results JSON file
Output:
- PDF file with plots showing metric trends over time

Example usage:
    python performance_plots.py results/ latest.json report.pdf
'''

from os import scandir
from sys import argv
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from json import load as j_load
from pathlib import Path

# Read serialized results from JSON file
def read_results(filename: str) -> dict[str: int]:
    with open(filename, 'r') as file:
        return {algo['name'] : algo['time'] for algo in j_load(file)}

def read_all_results(directory: str) -> list[dict[str, int]]:
    '''Read all results from directory sorted by recency.

    Args:
        directory: Path to directory with JSON results files

    Returns:
        List of results dictionaries, most recent first
    '''
    all_results = []
    try:
        # Sort numerically by filename (without extension)
        for fd in sorted(
            scandir(directory), 
            key=lambda x: int(Path(x.name).stem), 
            reverse=True
        ):
            if fd.is_file() and fd.name.endswith('.json'):
                all_results.append(read_results(fd.path))
    except (FileNotFoundError, PermissionError) as e:
        raise RuntimeError(f'Failed to scan results directory {directory}') from e

    return all_results

def build_plot(results: list[dict[str, int]], baseline: dict[str, int], name: str,
               pages: PdfPages) -> None:
    '''Build and save a plot for specific algorithm.

    Args:
        results: List of test results
        baseline: Baseline test results (last successful run)
        name: Name of the algorithm
        pages: PdfPages object to save the plot to
    '''
    points = []
    for res in results:
        points.append(res.get(name, 0) / 1000)

    fig, ax = plt.subplots()
    ax.stairs(points, fill=True, label='Results')
    ax.set_title(name)
    ax.set_xlabel('Run number')
    ax.set_ylabel('Time, s')
    ax.grid(visible=True, linestyle='--', alpha=0.7)

    if name in baseline:
        ax.stairs([baseline[name] / 1000 for i in range(len(results))], hatch='//',
                  label='Baseline')

    ax.legend()
    pages.savefig(fig)
    plt.close(fig)

def main() -> None:
    '''Main function to generate performance plots.

    Expects 4 command line arguments:
    1. Directory with test results
    2. Baseline results JSON
    3. Current run results JSON
    4. Output PDF filename
    '''
    if len(argv) != 5:
        print('Usage: python3 display_benchmarks.py <old_results> <baseline.json> '
              '<curr_result.json> <out.pdf>')
        exit(1)

    results = read_all_results(argv[1])
    baseline = read_results(argv[2])
    last_res = read_results(argv[3])
    results.append(last_res)

    with PdfPages(argv[4]) as pdf:
        for name in last_res:
            build_plot(results, baseline, name, pdf)

if __name__ == '__main__':
    main()
