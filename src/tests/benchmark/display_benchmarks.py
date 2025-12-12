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
from collections import namedtuple
import json
from pathlib import Path
from dataclasses import dataclass

import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

Result = dict[str:str]


@dataclass
class Results:
    date: str
    results: list[Result]


# Read serialized results from JSON file
def read_results(filename: str) -> Results:
    with open(filename, 'r') as file:
        json_file = json.load(file)
        date = json_file['date']
        results = json_file['results']
        return Results(date, {algo['name']: algo['time'] for algo in results})


def read_all_results(directory: str) -> list[Results]:
    '''Read all results from directory sorted by recency.

    Args:
        directory: Path to directory with JSON results files

    Returns:
        List of results objects, most recent first
    '''
    all_results = []
    try:
        # Sort numerically by filename (without extension)
        for fd in sorted(scandir(directory),
                         key=lambda x: int(Path(x.name).stem),
                         reverse=True):
            if fd.is_file() and fd.name.endswith('.json'):
                all_results.append(read_results(fd.path))
    except (FileNotFoundError, PermissionError) as e:
        raise RuntimeError(
            f'Failed to scan results directory {directory}') from e

    return all_results


def build_plot(results: list[Results], baseline: Results, name: str,
               pages: PdfPages) -> None:
    '''Build and save a plot for specific algorithm.

    Args:
        results: List of test results
        baseline: Baseline test results (last successful run)
        name: Name of the algorithm
        pages: PdfPages object to save the plot to
    '''
    dates = []
    points = []
    for res in results:
        dates.append(res.date)
        points.append(res.results.get(name, 0) / 1000)

    fig, ax = plt.subplots()
    # Dates are not guaranteed to be unique, so tick_label must be used
    ax.bar(range(len(points)),
           points,
           tick_label=dates,
           fill=True,
           label='Results')
    ax.set_title(name)
    ax.set_xlabel('Date')
    ax.set_ylabel('Time, s')
    plt.xticks(rotation=45)
    ax.grid(visible=True, linestyle='--', alpha=0.7)

    if name in baseline.results:
        ax.axhline(baseline.results[name] / 1000,
                   color='green',
                   label='Baseline')

    ax.legend()
    # Padding is needed for date to be displayed correctly
    pages.savefig(fig, bbox_inches='tight', pad_inches=0.15)
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
        print(
            'Usage: python3 display_benchmarks.py <old_results> <baseline.json> '
            '<curr_result.json> <out.pdf>')
        exit(1)

    results = read_all_results(argv[1])
    baseline = read_results(argv[2])
    last_res = read_results(argv[3])
    results.append(last_res)

    with PdfPages(argv[4]) as pdf:
        for name in last_res.results:
            build_plot(results, baseline, name, pdf)


if __name__ == '__main__':
    main()
