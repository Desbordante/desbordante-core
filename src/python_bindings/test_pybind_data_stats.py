import unittest
import desbordante as db

DATASET_PATH = "TestDataStats.csv"
SEPARATOR = ','
HAS_HEADER = False


class TestDataStats(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data_stats = db.statistics.algorithms.DataStats()
        cls.data_stats.load_data(table=(DATASET_PATH, SEPARATOR, HAS_HEADER))
        cls.data_stats.execute()

        return super().setUpClass()

    def test_null_empties(self) -> None:
        self.assertIsNone(self.data_stats.get_min(0))
        self.assertIsNone(self.data_stats.get_max(0))
        self.assertIsNone(self.data_stats.get_sum(0))
        self.assertIsNone(self.data_stats.get_average(0))
        self.assertIsNone(self.data_stats.get_corrected_std(0))
        self.assertIsNone(self.data_stats.get_skewness(0))
        self.assertIsNone(self.data_stats.get_kurtosis(0))
        self.assertIsNone(self.data_stats.get_quantile(0.25, 0))
        self.assertIsNone(self.data_stats.get_quantile(0.5, 0))
        self.assertIsNone(self.data_stats.get_quantile(0.75, 0))
        self.assertIsNone(self.data_stats.get_number_of_zeros(0))
        self.assertIsNone(self.data_stats.get_number_of_negatives(0))
        self.assertIsNone(self.data_stats.get_sum_of_squares(0))
        self.assertIsNone(self.data_stats.get_geometric_mean(0))
        self.assertIsNone(self.data_stats.get_mean_ad(0))
        self.assertIsNone(self.data_stats.get_median(0))
        self.assertIsNone(self.data_stats.get_median_ad(0))

    def test_get_number_of_values(self) -> None:
        res = self.data_stats.get_number_of_values(0)
        expected = 0
        self.assertEqual(expected, res)

    def test_get_number_of_columns(self) -> None:
        res = self.data_stats.get_number_of_columns()
        expected = 11
        self.assertEqual(expected, res)

    def test_get_null_columns(self) -> None:
        res = self.data_stats.get_null_columns()
        expected = []
        self.assertEqual(expected, res)

    def test_get_columns_with_null(self) -> None:
        res = self.data_stats.get_columns_with_null()
        expected = [0, 1, 2]
        self.assertEqual(expected, res)

    def test_get_columns_with_all_unique_values(self) -> None:
        res = self.data_stats.get_columns_with_all_unique_values()
        expected = [8, 9, 10]
        self.assertEqual(expected, res)

    def test_get_number_of_distinct(self) -> None:
        res = self.data_stats.get_number_of_distinct(5)
        expected = 6
        self.assertEqual(expected, res)

    def test_is_categorical(self) -> None:
        res = self.data_stats.is_categorical(3, 5)
        self.assertTrue(res)

    def test_get_average(self) -> None:
        res = self.data_stats.get_average(2)
        expected = 53.1525
        self.assertAlmostEqual(expected, res)

    def test_get_corrected_std(self) -> None:
        res = self.data_stats.get_corrected_std(7)
        expected = 335.7594687731
        self.assertAlmostEqual(expected, res)

    def test_get_skewness(self) -> None:
        res = self.data_stats.get_skewness(7)
        expected = 1.166303561798
        self.assertAlmostEqual(expected, res)

    def test_get_kurtosis(self) -> None:
        res = self.data_stats.get_kurtosis(8)
        expected = 1.05882994629
        self.assertAlmostEqual(expected, res)

    def test_get_min(self) -> None:
        res = self.data_stats.get_min(8)
        expected = -2841
        self.assertEqual(expected, res)

    def test_get_max(self) -> None:
        res = self.data_stats.get_max(8)
        expected = 9840
        self.assertEqual(expected, res)

    def test_get_sum(self) -> None:
        res = self.data_stats.get_sum(2)
        expected = 212.61
        self.assertAlmostEqual(expected, res)

    def test_get_quantile(self) -> None:
        quantile_0_25 = self.data_stats.get_quantile(0.25, 4)
        quantile_0_5 = self.data_stats.get_quantile(0.5, 4)
        quantile_0_75 = self.data_stats.get_quantile(0.75, 4)

        self.assertEqual(2, quantile_0_25)
        self.assertEqual(3, quantile_0_5)
        self.assertEqual(4, quantile_0_75)

    def test_get_number_of_zeros(self) -> None:
        res = self.data_stats.get_number_of_zeros(7)
        expected = 3
        self.assertEqual(expected, res)

    def test_get_number_of_negatives(self) -> None:
        res = self.data_stats.get_number_of_negatives(8)
        expected = 3
        self.assertEqual(expected, res)

    def test_get_sum_of_squares(self) -> None:
        res = self.data_stats.get_sum_of_squares(7)
        expected = 1096089.607224
        self.assertAlmostEqual(expected, res)

    def test_get_geometric_mean(self) -> None:
        res = self.data_stats.get_geometric_mean(9)
        expected = 33.33024629230983
        self.assertAlmostEqual(expected, res)

    def test_get_mean_ad(self) -> None:
        res = self.data_stats.get_mean_ad(7)
        expected = 258.263
        self.assertAlmostEqual(expected, res)

    def test_get_median(self) -> None:
        res = self.data_stats.get_median(9)
        expected = 25.875
        self.assertAlmostEqual(expected, res)

    def test_get_median_ad(self) -> None:
        res = self.data_stats.get_median_ad(8)
        expected = 123
        self.assertAlmostEqual(expected, res)

    def test_get_num_nulls(self) -> None:
        res = self.data_stats.get_num_nulls(0)
        expected = 5
        self.assertEqual(expected, res)

if __name__ == "__main__":
    unittest.main()
