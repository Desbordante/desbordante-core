# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "click>=8.2.0, <9",
#     "matplotlib>=3.8.0, <4",
#     "pydantic>=2.10.0, <3"
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

See python3 display_benchmarks.py --help for more info.
'''

from os import scandir
from sys import argv
from collections import namedtuple
from pathlib import Path
from dataclasses import dataclass
from typing import Any, Annotated
from datetime import timedelta, date

import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import click
from pydantic import BaseModel, BeforeValidator, PlainSerializer

MillisTimeDelta = Annotated[
    timedelta,
    BeforeValidator(lambda v: timedelta(milliseconds=v)),
    PlainSerializer(lambda td: td / timedelta(milliseconds=1), return_type=int
                    ),
]


class Result(BaseModel):
    name: str
    time: MillisTimeDelta


class Results(BaseModel):
    date: date
    results: list[Result]


# Read serialized results from JSON file
def read_results(filename: str) -> Results:
    with open(filename, 'r') as file:
        return Results.model_validate_json(file.read())


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
        time = timedelta(0)
        for record in res.results:
            if record.name == name:
                time = record.time
                break
        points.append(time.seconds)

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
        for record in baseline.results:
            if record.name == name:
                ax.axhline(record.time.seconds,
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
        for record in last_res.results:
            build_plot(results, baseline_results, record.name, pdf)


if __name__ == '__main__':
    main()
