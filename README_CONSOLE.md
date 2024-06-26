<p>
   <img src="https://github.com/Mstrutov/Desbordante/assets/88928096/d687809b-5a3b-420e-a192-a1a2b6697b2a"/>
</p>

---

# Desbordante: high-performance data profiler (console interface)

## What is it?

[**Desbordante**](https://github.com/Desbordante/desbordante-core) is a high-performance data profiler oriented towards exploratory data analysis. This is the repository for the Desbordante console interface, which is published as a separate [package](https://pypi.org/project/desbordante-cli/). This package depends on the [desbordante package](https://pypi.org/project/desbordante/), which contains the C++ code for pattern discovery and validation. As the result, depending on the algorithm and dataset, the runtimes may be cut by 2-10 times compared to the alternative tools.

## Table of Contents

- [Main Features](#main-features)
- [Installation](#installation)
- [Usage Examples](#usage-examples)
- [Contacts and Q&A](#contacts-and-qa)

# Main Features

[**Desbordante**](https://github.com/Desbordante/desbordante-core) is a high-performance data profiler that is capable of discovering and validating many different patterns in data using various algorithms. 

The **Discovery** task is designed to identify all instances of a specified pattern *type* of a given dataset.

The **Validation** task is different: it is designed to check whether a specified pattern *instance* is present in a given dataset. This task not only returns True or False, but it also explains why the instance does not hold (e.g. it can list table rows with conflicting values).

The currently supported data patterns are:
* Functional dependency variants:
    - Exact functional dependencies (discovery and validation)
    - Approximate functional dependencies, with g<sub>1</sub> metric (discovery and validation)
    - Probabilistic functional dependencies, with PerTuple and PerValue metrics (discovery)
* Graph functional dependencies (validation)
* Conditional functional dependencies (discovery)
* Inclusion dependencies (discovery)
* Order dependencies:
   - set-based axiomatization (discovery)
   - list-based axiomatization (discovery)
* Metric functional dependencies (validation)
* Fuzzy algebraic constraints (discovery)
* Unique column combinations:
   - Exact unique column combination (discovery and validation)
   - Approximate unique column combination, with g<sub>1</sub> metric (discovery and validation)
* Association rules (discovery)

For more information about the supported patterns check the main [repo](https://github.com/Desbordante/desbordante-core).

## Installation

**Requrements**:
* Python 3.11+
* pipx
* [`desbordante` package](https://pypi.org/project/desbordante/) requirements

### PyPI
Run the following command:
```sh
pipx install desbordante-cli
```
### Git
```sh
pipx install git+https://github.com/desbordante/desbordante-cli
```

## Usage examples
Example datasets can be found at main [repo](https://github.com/Desbordante/desbordante-core)

1) Discover all exact functional dependencies in a table stored in a comma-separated file with a header row. In this example the default FD discovery algorithm (HyFD) is used.

```sh
desbordante --task=fd --table=../examples/datasets/university_fd.csv , True
```

```text
[Course Classroom] -> Professor
[Classroom Semester] -> Professor
[Classroom Semester] -> Course
[Professor] -> Course
[Professor Semester] -> Classroom
[Course Semester] -> Classroom
[Course Semester] -> Professor
```

2) Discover all approximate functional dependencies with error less than or equal to 0.1 in a table represented by a .csv file that uses a comma as the separator and has a header row. In this example the default AFD discovery algorithm (Pyro) is used.

```sh
desbordante --task=afd --table=../examples/datasets/inventory_afd.csv , True --error=0.1
```

```text
[Id] -> ProductName
[Id] -> Price
[ProductName] -> Price
```

3) Check whether metric functional dependency “Title -> Duration” with radius 5 (using the Euclidean metric) holds in a table represented by a .csv file that uses a comma as the separator and has a header row. In this example the default MFD validation algorithm (BRUTE) is used.

```sh
desbordante --task=mfd_verification --table=../examples/datasets/theatres_mfd.csv , True --lhs_indices=0 --rhs_indices=2 --metric=euclidean --parameter=5
```

```text
True
```

For more information check the --help option:
```sh
desbordante --help
```

# Contacts and Q&A

If you have any questions regarding the tool you can create an [issue](https://github.com/Desbordante/desbordante-cli/issues) at GitHub.
