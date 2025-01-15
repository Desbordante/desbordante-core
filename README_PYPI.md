<p>
   <img src="https://github.com/Mstrutov/Desbordante/assets/88928096/d687809b-5a3b-420e-a192-a1a2b6697b2a"/>
</p>

---

# Desbordante: high-performance data profiler

## What is it?

**Desbordante** is a high-performance data profiler oriented towards exploratory data analysis

Try the web version at https://desbordante.unidata-platform.ru/

## Table of Contents

- [Main Features](#main-features)
- [Usage Examples](#usage-examples)
- [I still don't understand how to use Desbordante and patterns :(](#i-still-dont-understand-how-to-use-Desbordante-and-patterns-)
- [Installation](#installation)
- [Installation from sources](#installation-from-sources)
- [Troubleshooting](#troubleshooting)
- [Cite](#cite)
- [Contacts and Q&A](#contacts-and-qa)

# Main Features

[**Desbordante**](https://github.com/Desbordante/desbordante-core) is a high-performance data profiler that is capable of discovering and validating many different patterns in data using various algorithms. 

The **Discovery** task is designed to identify all instances of a specified pattern *type* of a given dataset.

The **Validation** task is different: it is designed to check whether a specified pattern *instance* is present in a given dataset. This task not only returns True or False, but it also explains why the instance does not hold (e.g. it can list table rows with conflicting values).

For some patterns Desbordante supports a **dynamic** task variant. The distiguishing feature of dynamic algorithms compared to classic (static) algorithms is that after a result is obtained, the table can be changed and a dynamic algorithm will update the result based just on those changes instead of processing the whole table again. As a result, they can be up to several orders of magnitude faster than classic (static) ones in some situations.

The currently supported data patterns are:
* Exact functional dependencies (discovery and validation)
* Approximate functional dependencies, with 
    - $g_1$ metric — classic AFDs (discovery and validation)
    - $\mu+$ metric (discovery)
    - $\tau$ metric (discovery)
    - $pdep$ metric (discovery)
    - $\rho$ metric (discovery)
* Probabilistic functional dependencies, with PerTuple and PerValue metrics (discovery and validation)
* Classic soft functional dependencies (with corellations), with $\rho$ metric (discovery and validation)
* Dynamic validation of exact and approximate ($g_1$) functional dependencies
* Numerical dependencies (validation)
* Graph functional dependencies (validation)
* Conditional functional dependencies (discovery)
* Inclusion dependencies
   - Exact inclusion dependencies (discovery and validation)
   - Approximate inclusion dependencies, with $g^{'}_{3}$ metric (discovery and validation)
* Order dependencies:
   - set-based axiomatization (discovery)
   - list-based axiomatization (discovery)
* Metric functional dependencies (validation)
* Fuzzy algebraic constraints (discovery)
* Differential Dependencies (discovery)
* Unique column combinations:
   - Exact unique column combination (discovery and validation)
   - Approximate unique column combination, with $g_1$ metric (discovery and validation)
* Association rules (discovery)
* Numerical association rules (discovery)
* Matching dependencies (discovery)
* Variable heterogeneous denial constraints (validation)

This package uses the library of the Desbordante platform, which is written in C++. This means that depending on the algorithm and dataset, the runtimes may be cut by 2-10 times compared to the alternatives.

## Usage examples

1) Discover all exact functional dependencies in a table stored in a comma-separated file with a header row. In this example the default FD discovery algorithm (HyFD) is used.


```python
import desbordante

TABLE = 'examples/datasets/university_fd.csv'

algo = desbordante.fd.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))
algo.execute()
result = algo.get_fds()
print('FDs:')
for fd in result:
    print(fd)
```

```text
FDs:
[Course Classroom] -> Professor
[Classroom Semester] -> Professor
[Classroom Semester] -> Course
[Professor] -> Course
[Professor Semester] -> Classroom
[Course Semester] -> Classroom
[Course Semester] -> Professor
```

2) Discover all approximate functional dependencies with error less than or equal to 0.1 in a table represented by a
   .csv file that uses a comma as the separator and has a header row. In this example the AFD discovery algorithm Pyro
   is used.

```python
import desbordante

TABLE = 'examples/datasets/inventory_afd.csv'
ERROR = 0.1

algo = desbordante.afd.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))
algo.execute(error=ERROR)
result = algo.get_fds()
print('AFDs:')
for fd in result:
    print(fd)
```

```text
AFDs:
[Id] -> Price
[Id] -> ProductName
[ProductName] -> Price
```

3) Check whether metric functional dependency “Title -> Duration” with radius 5 (using the Euclidean metric) holds in a
   table represented by a .csv file that uses a comma as the separator and has a header row. In this example the default
   MFD validation algorithm (BRUTE) is used.

```python
import desbordante

TABLE = 'examples/datasets/theatres_mfd.csv'
METRIC = 'euclidean'
LHS_INDICES = [0]
RHS_INDICES = [2]
PARAMETER = 5

algo = desbordante.mfd_verification.algorithms.Default()
algo.load_data(table=(TABLE, ',', True))
algo.execute(lhs_indices=LHS_INDICES, metric=METRIC,
             parameter=PARAMETER, rhs_indices=RHS_INDICES)
if algo.mfd_holds():
    print('MFD holds')
else:
    print('MFD does not hold')
```

```text
MFD holds
```

4) Discover approximate functional dependencies with various error thresholds. Here, we are using a pandas DataFrame to load data from a CSV file.

```python-repl
>>> import desbordante
>>> import pandas as pd
>>> pyro = desbordante.afd.algorithms.Pyro()  # same as desbordante.afd.algorithms.Default()
>>> df = pd.read_csv('examples/datasets/iris.csv', sep=',', header=None)
>>> pyro.load_data(table=df)
>>> pyro.execute(error=0.0)
>>> print(f'[{", ".join(map(str, pyro.get_fds()))}]')
[[0 1 2] -> 4, [0 2 3] -> 4, [0 1 3] -> 4, [1 2 3] -> 4]
>>> pyro.execute(error=0.1)
>>> print(f'[{", ".join(map(str, pyro.get_fds()))}]')
[[2] -> 0, [2] -> 3, [2] -> 1, [0] -> 2, [3] -> 0, [0] -> 3, [0] -> 1, [1] -> 3, [1] -> 0, [3] -> 2, [3] -> 1, [1] -> 2, [2] -> 4, [3] -> 4, [0] -> 4, [1] -> 4]
>>> pyro.execute(error=0.2)
>>> print(f'[{", ".join(map(str, pyro.get_fds()))}]')
[[2] -> 0, [0] -> 2, [3] -> 2, [1] -> 2, [2] -> 4, [3] -> 4, [0] -> 4, [1] -> 4, [3] -> 0, [1] -> 0, [2] -> 3, [2] -> 1, [0] -> 3, [0] -> 1, [1] -> 3, [3] -> 1]
>>> pyro.execute(error=0.3)
>>> print(f'[{", ".join(map(str, pyro.get_fds()))}]')
[[2] -> 1, [0] -> 2, [2] -> 0, [2] -> 3, [0] -> 1, [3] -> 2, [3] -> 1, [1] -> 2, [3] -> 0, [0] -> 3, [4] -> 1, [1] -> 0, [1] -> 3, [4] -> 2, [4] -> 3, [2] -> 4, [3] -> 4, [0] -> 4, [1] -> 4]
```

More examples can be found in the [Desbordante repository](https://github.com/Desbordante/desbordante-core/tree/main/examples) on GitHub.

## I still don't understand how to use Desbordante and patterns :(

No worries! Desbordante offers a novel type of data profiling, which may require that you first familiarize yourself with its concepts and usage. The most challenging part of Desbordante are the primitives: their definitions and applications in practice. To help you get started, here’s a step-by-step guide:

1) First of all, explore the guides on our [website](https://desbordante.unidata-platform.ru/papers). Since our team currently does not include technical writers, it's possible that some guides may be missing.
2) To compensate for the lack of guides, we provide several examples for each supported pattern. These examples illustrate both the pattern itself and how to use it in Python. You can check them out [here](https://github.com/Desbordante/desbordante-core/tree/main/examples).
3) Each of our patterns was introduced in a research paper. These papers typically provide a formal definition of the pattern, examples of use, and its application scope. We recommend at least skimming through them. Don't be discouraged by the complexity of the papers! To effectively use the patterns, you only need to read the more accessible parts, such as the introduction and the example sections.
4) Finally, do not hesitate to ask questions in the mailing list (link below) or create an issue.

### Papers about patterns

Here is a list of papers about patterns, organized in the recommended reading order in each item:

* Exact functional dependencies
   - [Thorsten Papenbrock et al. 2015. Functional dependency discovery: an experimental evaluation of seven algorithms. Proc. VLDB Endow. 8, 10 (June 2015), 1082–1093.](http://www.vldb.org/pvldb/vol8/p1082-papenbrock.pdf)
   - [Thorsten Papenbrock and Felix Naumann. 2016. A Hybrid Approach to Functional Dependency Discovery. In Proceedings of the 2016 International Conference on Management of Data (SIGMOD '16). Association for Computing Machinery, New York, NY, USA, 821–833.](https://hpi.de/fileadmin/user_upload/fachgebiete/naumann/publications/PDFs/2016_papenbrock_a.pdf)
* Approximate functional dependencies ($g_{1}, \mu+, \tau, pdep, \rho$ metrics)
   - [Marcel Parciak et al. 2024. Measuring Approximate Functional Dependencies: A Comparative Study. In Proceedings 2024 IEEE 40th International Conference on Data Engineering (ICDE), Utrecht, Netherlands, 2024, pp. 3505-3518](https://arxiv.org/abs/2312.06296)
   - [Sebastian Kruse and Felix Naumann. 2018. Efficient discovery of approximate dependencies. Proc. VLDB Endow. 11, 7 (March 2018), 759–772.](https://www.vldb.org/pvldb/vol11/p759-kruse.pdf)
   - [Yka Huhtala et al. 1999. TANE: An Efficient Algorithm for Discovering Functional and Approximate Dependencies. Comput. J. 42(2): 100-111](https://dm-gatech.github.io/CS8803-Fall2018-DML-Papers/tane.pdf)
* Probabilistic functional dependencies ($PerTuple$ and $PerValue$ metrics)
   - [Daisy Zhe Wang et al. Functional Dependency Generation and Applications in Pay-As-You-Go Data Integration Systems. WebDB 2009](http://webdb09.cse.buffalo.edu/papers/Paper18/webdb09.pdf)
   - [Daisy Zhe Wang et al. Discovering Functional Dependencies in Pay-As-You-Go Data Integration Systems. Tech Rep. UCB/EECS-2009-119.](https://www2.eecs.berkeley.edu/Pubs/TechRpts/2009/EECS-2009-119.pdf)
* Classic soft functional dependencies ($\rho$ metric)
   - [Ihab F. Ilyas et al. 2004. CORDS: automatic discovery of correlations and soft functional dependencies. In Proceedings of the 2004 ACM SIGMOD international conference on Management of data (SIGMOD '04). Association for Computing Machinery, New York, NY, USA, 647–658. ](https://cs.uwaterloo.ca/~ilyas/papers/cords.pdf)
* Numerical Dependencies
   - [Paolo Ciaccia et al. 2013. Efficient derivation of numerical dependencies. Information Systems, Volume 38, Issue 3. Pages 410-429.](https://www.sciencedirect.com/science/article/abs/pii/S0306437912001044)
* Graph functional dependencies
    - [Wenfei Fan, Yinghui Wu, and Jingbo Xu. 2016. Functional Dependencies for Graphs. In Proceedings of the 2016 International Conference on Management of Data (SIGMOD '16). Association for Computing Machinery, New York, NY, USA, 1843–1857.](https://dl.acm.org/doi/pdf/10.1145/2882903.2915232)
* Conditional functional dependencies
    - [Rammelaere, J., Geerts, F. (2019). Revisiting Conditional Functional Dependency Discovery: Splitting the “C” from the “FD”. Machine Learning and Knowledge Discovery in Databases. ECML PKDD 2018. ](https://link.springer.com/chapter/10.1007/978-3-030-10928-8_33)
* Exact and approximate inclusion dependencies
    - [Falco Dürsch et al. 2019. Inclusion Dependency Discovery: An Experimental Evaluation of Thirteen Algorithms. In Proceedings of the 28th ACM International Conference on Information and Knowledge Management (CIKM '19). Association for Computing Machinery, New York, NY, USA, 219–228.](https://hpi.de/fileadmin/user_upload/fachgebiete/naumann/publications/PDFs/2019_duersch_inclusion.pdf)
    - [Sebastian Kruse, et al. Fast Approximate Discovery of Inclusion Dependencies. BTW 2017: 207-226](http://btw2017.informatik.uni-stuttgart.de/slidesandpapers/F4-10-47/paper_web.pdf)
    - [Marchi, F.D., Lopes, S. & Petit, JM. Unary and n-ary inclusion dependency discovery in relational databases. J Intell Inf Syst 32, 53–73 (2009)](https://liris.cnrs.fr/Documents/Liris-3034.pdf)
* Order dependencies:
   - [Jaroslaw Szlichta et al. 2017. Effective and complete discovery of order dependencies via set-based axiomatization. Proc. VLDB Endow. 10, 7 (March 2017), 721–732.](http://www.vldb.org/pvldb/vol10/p721-szlichta.pdf)
   - [Langer, P., Naumann, F. Efficient order dependency detection. The VLDB Journal 25, 223–241 (2016)](https://link.springer.com/article/10.1007/s00778-015-0412-3)
* Metric functional dependencies
   - [N. Koudas et al. "Metric Functional Dependencies," 2009 IEEE 25th International Conference on Data Engineering, Shanghai, China, 2009, pp. 1275-1278.](https://ieeexplore.ieee.org/document/4812519)
* Fuzzy algebraic constraints
   - [Paul G. Brown and Peter J. Hass. 2003. BHUNT: automatic discovery of Fuzzy algebraic constraints in relational data. In Proceedings of the 29th international conference on Very large data bases - Volume 29 (VLDB '03), Vol. 29. VLDB Endowment, 668–679.](https://www.vldb.org/conf/2003/papers/S20P03.pdf)
* Differential dependencies
   - [Shaoxu Song and Lei Chen. 2011. Differential dependencies: Reasoning and discovery. ACM Trans. Database Syst. 36, 3, Article 16 (August 2011), 41 pages.](https://sxsong.github.io/doc/11tods.pdf)
* Exact and approximate unique column combinations
   - [Sebastian Kruse and Felix Naumann. 2018. Efficient discovery of approximate dependencies. Proc. VLDB Endow. 11, 7 (March 2018), 759–772.](https://www.vldb.org/pvldb/vol11/p759-kruse.pdf)
* Association rules
   - [Charu C. Aggarwal, Jiawei Han. 2014. Frequent Pattern Mining. Springer Cham. pp 471.](https://link.springer.com/book/10.1007/978-3-319-07821-2)
* Numerical association rules
   - [Minakshi Kaushik, Rahul Sharma, Iztok Fister Jr., and Dirk Draheim. 2023. Numerical Association Rule Mining: A Systematic Literature Review. 1, 1 (July 2023), 50 pages.](https://arxiv.org/abs/2307.00662)
   - [Fister, Iztok & Fister jr, Iztok. 2020. uARMSolver: A framework for Association Rule Mining. 10.48550/arXiv.2010.10884.](https://doi.org/10.48550/arXiv.2010.10884)
* Matching dependencies
   - [Philipp Schirmer, Thorsten Papenbrock, Ioannis Koumarelas, and Felix Naumann. 2020. Efficient Discovery of Matching Dependencies. ACM Trans. Database Syst. 45, 3, Article 13 (September 2020), 33 pages. https://doi.org/10.1145/3392778](https://dl.acm.org/doi/10.1145/3392778)
* Denial constraints
   - [X. Chu, I. F. Ilyas and P. Papotti. Holistic data cleaning: Putting violations into context. 2013. IEEE 29th International Conference on Data Engineering (ICDE), Brisbane, QLD, Australia, 2013, pp. 458-469,](https://cs.uwaterloo.ca/~ilyas/papers/XuICDE2013.pdf)
   - [Zifan Liu, Shaleen Deep, Anna Fariha, Fotis Psallidas, Ashish Tiwari, and Avrilia Floratou. 2024. Rapidash: Efficient Detection of Constraint Violations. Proc. VLDB Endow. 17, 8 (April 2024), 2009–2021.](https://arxiv.org/pdf/2309.12436)

## Installation

The source code is currently hosted on GitHub at https://github.com/Desbordante/desbordante-core

Wheels for the latest released version are available at the Python Package Index (PyPI).

**Currently only manylinux2014 (Ubuntu 20.04+, or any other linux distribution with gcc 10+) is supported**.

```bash
$ pip install desbordante
 ```

## Installation from sources

Install all dependencies listed in [README.md](https://github.com/Desbordante/desbordante-core/blob/main/README.md).

Then, in the Desbordante directory (the same one that contains this file), execute:

```bash
./build.sh
python3 -m venv venv
source venv/bin/activate
python3 -m pip install .
```

## Troubleshooting
### No type hints in IDE
If type hints don't work for you in Visual Studio Code, for example, then install stubs using the command:
```sh
pip install desbordate-stubs
```
**NOTE**: Stubs may not fully support current version of `desbordante` package, as they are updated independently.

## Cite

If you use this software for research, please cite one of our papers:

1) George Chernishev, et al. Solving Data Quality Problems with Desbordante: a Demo. CoRR abs/2307.14935 (2023).
2) George Chernishev, et al. "Desbordante: from benchmarking suite to high-performance science-intensive data profiler (preprint)". CoRR abs/2301.05965. (2023).
3) M. Strutovskiy, N. Bobrov, K. Smirnov and G. Chernishev, "Desbordante: a Framework for Exploring Limits of Dependency Discovery Algorithms," 2021 29th Conference of Open Innovations Association (FRUCT), 2021, pp. 344-354, doi: 10.23919/FRUCT52173.2021.9435469.
4) A. Smirnov, A. Chizhov, I. Shchuckin, N. Bobrov and G. Chernishev, "Fast Discovery of Inclusion Dependencies with Desbordante," 2023 33rd Conference of Open Innovations Association (FRUCT), Zilina, Slovakia, 2023, pp. 264-275, doi: 10.23919/FRUCT58615.2023.10143047.
5) Y. Kuzin, D. Shcheka, M. Polyntsov, K. Stupakov, M. Firsov and G. Chernishev, "Order in Desbordante: Techniques for Efficient Implementation of Order Dependency Discovery Algorithms," 2024 35th Conference of Open Innovations Association (FRUCT), Tampere, Finland, 2024, pp. 413-424.
6) I. Barutkin, M. Fofanov, S. Belokonny, V. Makeev and G. Chernishev, "Extending Desbordante with Probabilistic Functional Dependency Discovery Support," 2024 35th Conference of Open Innovations Association (FRUCT), Tampere, Finland, 2024, pp. 158-169.
7) A. Shlyonskikh, M. Sinelnikov, D. Nikolaev, Y. Litvinov and G. Chernishev, "Lightning Fast Matching Dependency Discovery with Desbordante," 2024 36th Conference of Open Innovations Association (FRUCT), Lappeenranta, Finland, 2024, pp. 729-740.


# Contacts and Q&A

If you have any questions regarding the tool usage you can ask it in
our [google group](https://groups.google.com/g/desbordante). To contact dev team email George Chernishev, Maxim
Strutovsky or Nikita Bobrov.
