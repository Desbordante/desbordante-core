# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "click>=8.2.0",
#     "matplotlib>=3.8.0",
# ]
# ///
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
from typing import TypeAlias
from datetime import timedelta

import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import click

Result: TypeAlias = dict[str:timedelta]


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
        return Results(date, {
            algo['name']: timedelta(milliseconds=algo['time'])
            for algo in results
        })


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


def build_plot(results: list[Results], baseline: Results | None, name: str,
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
        points.append(res.results.get(name, timedelta(0)).seconds)

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

    if baseline:
        if name in baseline.results:
            ax.axhline(baseline.results[name].seconds,
                       color='green',
                       label='Baseline')

    ax.legend()
    # Padding is needed for date to be displayed correctly
    pages.savefig(fig, bbox_inches='tight', pad_inches=0.15)
    plt.close(fig)


@click.command()
@click.option('--old_results',
              '-R',
              'old_results',
              type=click.Path(exists=True, file_okay=False, dir_okay=True),
              required=True,
              help='Directory with previous test results')
@click.option('--curr_results',
              '-r',
              'curr_results',
              type=click.Path(exists=True, file_okay=True, dir_okay=False),
              required=True,
              help='Current test results')
@click.option('--output',
              '-o',
              'output',
              type=click.Path(exists=False, file_okay=True, dir_okay=False),
              required=True,
              help='Output PDF filename')
@click.option('--baseline',
              '-b',
              'baseline',
              type=click.Path(exists=True, file_okay=True, dir_okay=False),
              help='Latest successful test results')
def main(old_results, curr_results, output, baseline) -> None:
    results = read_all_results(old_results)
    baseline_results = read_results(baseline) if baseline else None
    last_res = read_results(curr_results)
    results.append(last_res)

    with PdfPages(output) as pdf:
        for name in last_res.results:
            build_plot(results, baseline_results, name, pdf)


if __name__ == '__main__':
    main()
