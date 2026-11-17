import pathlib
import tempfile
import unittest
from collections import namedtuple
from itertools import chain
from pathlib import Path

import desbordante as desb

OptionContainer = namedtuple("OptionContainer", ['path', 'load_options', 'execute_options'])
FailureCaseContainer = namedtuple("FailureCaseContainer", ['path', 'options'])

TABLE_ONLY_CONTAINER = OptionContainer(
    "WDC_satellites.csv", {}, {}
)


def get_common_option_container(execute_options):
    return OptionContainer(
        "WDC_satellites.csv", {}, execute_options
    )


def get_apriori_load_container(load_options):
    return OptionContainer("TestWide.csv", load_options, {})


def check_metric_verifier_failure(dataset, options) -> bool:
    alg = desb.mfd_verification.algorithms.MetricVerifier()
    alg.load_data(table=(dataset, ",", True))
    for opt_name in options:
        alg._set_option(opt_name, options[opt_name])


ALGO_CORRECT_OPTIONS_INFO = [
    (desb.fd.algorithms.Depminer, [TABLE_ONLY_CONTAINER]),
    (desb.fd.algorithms.FUN, [TABLE_ONLY_CONTAINER]),
    (desb.fd.algorithms.FdMine, [TABLE_ONLY_CONTAINER]),
    (desb.fd.algorithms.HyFD, [TABLE_ONLY_CONTAINER]),
    (desb.fd.algorithms.EulerFD, [TABLE_ONLY_CONTAINER]),
    (desb.afd.algorithms.Pyro, [
        get_common_option_container(
            {"seed": 1, "max_lhs": 12, "threads": 5, "error": 0.015}
        ),
    ]),
    (desb.fd.algorithms.DFD, [
        get_common_option_container(
            {
                "threads": 15,
            }
        ),
    ]),
    (desb.fd.algorithms.FastFDs, [
        get_common_option_container({"max_lhs": 12, "threads": 15}),
    ]),
    (desb.afd.algorithms.Tane, [
        get_common_option_container({"max_lhs": 12, "error": 0.015}),
    ]),
    (desb.statistics.algorithms.DataStats, [
        get_common_option_container({"threads": 15}),
    ]),
    (desb.ucc.algorithms.HyUCC, [
        get_common_option_container({"threads": 15}),
    ]),
    (desb.fd.algorithms.EulerFD, [
        get_common_option_container({"custom_random_seed": 102}),
    ]),
    (desb.fd_verification.algorithms.FDVerifier, [
        get_common_option_container(
            {"lhs_indices": [1, 2, 3], "rhs_indices": [1, 2, 3]}
        ),
    ]),
    (desb.ar.algorithms.Apriori, [
        get_apriori_load_container({"input_format": "tabular", "has_tid": True}),
        get_apriori_load_container({"input_format": "tabular", "has_tid": False}),
        get_apriori_load_container(
            {"input_format": "singular", "tid_column_index": 0, "item_column_index": 2}
        ),
        OptionContainer(
            "rules-kaggle-rows.csv",
            {
                "input_format": "singular",
                "tid_column_index": 0,
                "item_column_index": 1,
            },
            {"minconf": 0.00312, "minsup": 0.2321},
        ),
    ]),
    (desb.mfd_verification.algorithms.MetricVerifier, [
        OptionContainer(
            "TestLong.csv",
            {},
            {
                "metric": "euclidean",
                "rhs_indices": [1, 2],
                "parameter": 213.213111,
                "dist_from_null_is_infinity": False,
                "metric_algorithm": "approx",
                "lhs_indices": [0, 1, 2],
            },
        ),
        OptionContainer(
            "WDC_satellites.csv",
            {},
            {
                "metric": "levenshtein",
                "rhs_indices": [1],
                "parameter": 213.213111,
                "dist_from_null_is_infinity": False,
                "metric_algorithm": "approx",
                "lhs_indices": [0, 1, 2],
            },
        ),
        OptionContainer(
            "WDC_satellites.csv",
            {},
            {
                "metric": "cosine",
                "rhs_indices": [1],
                "parameter": 213.213111,
                "dist_from_null_is_infinity": False,
                "q": 123,
                "metric_algorithm": "approx",
                "lhs_indices": [0, 1, 2],
            },
        ),
    ]),
    (desb.pac_verification.algorithms.DomainPACVerifier, [
        OptionContainer(
            "TestLong.csv",
            {
                "column_indices": [0, 1],
                "domain": desb.pac.domains.Parallelepiped(["0", "0"], ["5", "5"], [1, 1.2]),
            },
            {
                "min_epsilon": 0,
                "max_epsilon": 7,
                "min_delta": 0.8,
                "diagonal_threshold": 1e-10,
            },
        ),
        OptionContainer(
            "TestLong.csv",
            {
                "column_indices": [1],
                "domain": desb.pac.domains.CustomDomain(
                    lambda val: int(val[0]) < 7, "[-\\infty, 7]"),
            },
            {
                "max_epsilon": 7,
                "min_delta": 0.8,
            },
        )
    ]),
]

METRIC_VERIFIER_FAILURE_CASES = [
    FailureCaseContainer(
        "WDC_satellites.csv",
        {
            "metric": "euclidean",
            "rhs_indices": [1, 2],
        }
    ),
    
    FailureCaseContainer(
        "TestLong.csv",
        {
            "metric": "euclidean",
            "q": 123,
        }
    ),
    FailureCaseContainer(
        "TestLong.csv",
        {
            "metric": "euclidean",
            "metric_algorithm": "brute",
        }
    ),
    FailureCaseContainer(
        "TestLong.csv",
        {"metric": "levenshtein", "rhs_indices": [1, 2]}
    ),
    FailureCaseContainer("WDC_satellites.csv", {"metric": "levenshtein", "q": 123}),
    FailureCaseContainer(
        "WDC_satellites.csv",
        {
            "metric": "levenshtein",
            "metric_algorithm": "brute",
        }
    ),
    FailureCaseContainer("TestLong.csv", {"metric": "levenshtein", "rhs_indices": [1]}),
    FailureCaseContainer("TestLong.csv", {"rhs_indices": [1]}),
    FailureCaseContainer("WDC_satellites.csv", {"rhs_indices": [1, 2]}),
    FailureCaseContainer("WDC_satellites.csv", {"q": 123}),
    FailureCaseContainer("WDC_satellites.csv", {"metric_algorithm": "approx"}),
]


class TestPythonBindings(unittest.TestCase):
    def _configure_and_verify_no_extras(self, testing_algo, params):
        while option_names := testing_algo._get_needed_options():
            for option_name in option_names:
                testing_algo._set_option(option_name, params.pop(option_name, None))

        self.assertEqual(params, {})

    def test_sd_miner_strategy_options_use_strings(self):
        miner = desb.sd.algorithms.SDMiner()

        self.assertEqual((str,), miner._get_option_type("interval_strategy"))
        self.assertEqual((str,), miner._get_option_type("assembly_strategy"))
        self.assertFalse(hasattr(desb.sd, "IntervalStrategy"))
        self.assertFalse(hasattr(desb.sd, "AssemblyStrategy"))

        miner.load_data(table=("TestLong.csv", ",", True))
        miner._set_option("interval_strategy", "approximate")
        miner._set_option("assembly_strategy", "greedy")
        options = miner.get_opts()
        self.assertEqual("approximate", options["interval_strategy"])
        self.assertEqual("greedy", options["assembly_strategy"])

    def test_sd_miner_sorted_row_indices_map_patterns_to_verifier_input(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            csv_path = Path(temp_dir) / "sd_miner_row_mapping.csv"
            csv_path.write_text("X,Y\n2,101\n1,100\n1,0\n", encoding="utf-8")

            miner = desb.sd.algorithms.SDMiner()
            miner.load_data(table=(str(csv_path), ",", True))
            miner.execute(
                lhs_indices=[0],
                rhs_indices=[1],
                g1=1.0,
                g2=1.0,
                minimum_confidence=1.0,
                minimum_support=2.0 / 3.0,
                interval_strategy="exact",
                assembly_strategy="exact",
            )

            sorted_row_indices = miner.get_sorted_row_indices()
            self.assertEqual([2, 1, 0], sorted_row_indices)
            tableau = miner.get_tableau()
            self.assertEqual(1, len(tableau))
            pattern = tableau[0]
            verifier_indices = sorted_row_indices[
                pattern.left_position:pattern.right_position + 1
            ]
            self.assertEqual([1, 0], verifier_indices)

            verifier = desb.sd_verification.algorithms.SDVerifier()
            verifier.load_data(table=(str(csv_path), ",", True))
            verifier.execute(
                lhs_indices=[0],
                rhs_indices=[1],
                indices=verifier_indices,
                g1=1.0,
                g2=1.0,
            )
            self.assertAlmostEqual(pattern.confidence, verifier.get_confidence())

    def test_sd_miner_repeated_execution_matches_fresh_instance(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            csv_path = Path(temp_dir) / "sd_miner_repeated.csv"
            csv_path.write_text("X,Y\n2,3\n1,0\n1,1\n3,4\n4,20\n", encoding="utf-8")
            table = (str(csv_path), ",", True)
            miner = desb.sd.algorithms.SDMiner()
            miner.load_data(table=table)

            def result(algo):
                return (
                    [(p.left, p.right, p.confidence) for p in algo.get_candidates()],
                    [(p.left_position, p.right_position, p.left_x, p.right_x,
                      p.support, p.confidence) for p in algo.get_tableau()],
                    algo.get_global_support(),
                    algo.get_sorted_row_indices(),
                )

            for interval, assembly, confidence, support, g1, g2 in [
                ("exact", "exact", 0.5, 1.0, 1.0, 2.0),
                ("exact", "exact", 1.0, 1.0, 9.0, 11.0),
                ("approximate", "greedy", 0.75, 0.0, 0.0, 1.0),
                ("approximate", "exact", 0.75, 1.0, 1.0, -1.0),
                ("exact", "greedy", 1.0, 1.0, 1.0, 2.0),
            ]:
                with self.subTest(interval=interval, assembly=assembly, support=support):
                    options = dict(lhs_indices=[0], rhs_indices=[1], g1=g1, g2=g2,
                                   minimum_confidence=confidence, minimum_support=support,
                                   interval_strategy=interval, assembly_strategy=assembly,
                                   delta=0.1)
                    miner.execute(**options)
                    fresh = desb.sd.algorithms.SDMiner()
                    fresh.load_data(table=table)
                    fresh.execute(**options)
                    self.assertEqual(result(miner), result(fresh))

    def _test_correct_option_setting(
        self, algo, path, separator, has_header, options: dict
    ):
        testing_algo = algo()
        self._configure_and_verify_no_extras(
                testing_algo,  {'table': (path, separator, has_header)} | options.load_options)
        testing_algo.load_data()

        self._configure_and_verify_no_extras(testing_algo, options.execute_options)

        algo_option_values = testing_algo.get_opts()
        for name, value in chain(options.execute_options.items(),
                                 options.load_options.items()):
            self.assertEqual(value, algo_option_values[name])

    def test_correct_load(self):
        for algo, option_containers in ALGO_CORRECT_OPTIONS_INFO:
            for option in option_containers:
                with self.subTest(msg=f"testing setting correct options for {algo.__name__}"):
                    self._test_correct_option_setting(
                        algo, option.path, ",", True, option
                    )

    def test_metric_verifier_failure_cases(self):
        for load in METRIC_VERIFIER_FAILURE_CASES:
            with self.subTest(msg=f"metric_verifier_load: {load}"):
                with self.assertRaises(desb.ConfigurationError):
                    check_metric_verifier_failure(load.path, load.options)
                


class TestMaxFEM(unittest.TestCase):
    # Sequence: event 1 three times, event 2 twice (infrequent at minsup=3),
    # window_size=1 prevents composite episodes.
    # Expected: one maximal episode [[1]] with support 3.
    _SEQUENCE = [[1], [2], [1], [2], [1]]
    _MINSUP = 3
    _WINDOW_SIZE = 1
    _EXPECTED = [([[1]], 3)]

    def _run(self, sequence_arg):
        alg = desb.fem.MaxFEM()
        alg.load_data(sequence=sequence_arg)
        alg.execute(minsup=self._MINSUP, window_size=self._WINDOW_SIZE)
        return alg.get_max_frequent_episodes()

    def test_iterable_input(self):
        result = self._run(self._SEQUENCE)
        self.assertEqual(result, self._EXPECTED)

    def test_path_input(self):
        content = "\n".join(" ".join(str(e) for e in events) for events in self._SEQUENCE)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
            f.write(content)
            tmp_path = pathlib.Path(f.name)
        try:
            result = self._run(tmp_path)
            self.assertEqual(result, self._EXPECTED)
        finally:
            tmp_path.unlink()


if __name__ == "__main__":
    unittest.main()
