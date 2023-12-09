import unittest
import desbordante as db

DATASET_PATH = "TestDataStats.csv"
SEPARATOR = ','
HAS_HEADER = True


class TestDataStats(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.data_stats = db.DataStats()
        cls.data_stats.load_data(DATASET_PATH, SEPARATOR, HAS_HEADER)
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
        res = self.data_stats.get_number_of_values(1)
        expected = 3
        self.assertEqual(expected, res)

    def test_get_number_of_columns(self) -> None:
        res = self.data_stats.get_number_of_columns()
        expected = 10
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
        expected = [6, 8, 9]
        self.assertEqual(expected, res)

    def test_get_number_of_distinct(self) -> None:
        res = self.data_stats.get_number_of_distinct(7)
        expected = 6
        self.assertEqual(expected, res)

    def test_is_categorical(self) -> None:
        res = self.data_stats.is_categorical(3, 5)
        self.assertTrue(res)

    def test_get_average(self) -> None:
        res = self.data_stats.get_average(2)
        expected = 70.5133333
        self.assertAlmostEqual(expected, res)

    def test_get_corrected_std(self) -> None:
        res = self.data_stats.get_corrected_std(7)
        expected = 352.4417534
        self.assertAlmostEqual(expected, res)

    def test_get_skewness(self) -> None:
        res = self.data_stats.get_skewness(7)
        expected = 0.97232238
        self.assertAlmostEqual(expected, res)

    def test_get_kurtosis(self) -> None:
        res = self.data_stats.get_kurtosis(8)
        expected = 0.293771
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
        res = self.data_stats.get_sum(9)
        expected = 1142.32
        self.assertAlmostEqual(expected, res)

    def test_get_quantile(self) -> None:
        res = self.data_stats.get_quantile(0.25, 9)
        expected = 10
        self.assertEqual(expected, res)

    def test_get_number_of_zeros(self) -> None:
        res = self.data_stats.get_number_of_zeros(7)
        expected = 2
        self.assertEqual(expected, res)

    def test_get_number_of_negatives(self) -> None:
        res = self.data_stats.get_number_of_negatives(8)
        expected = 2
        self.assertEqual(expected, res)

    def test_get_sum_of_squares(self) -> None:
        res = self.data_stats.get_sum_of_squares(8)
        expected = 105729737
        self.assertEqual(expected, res)

    def test_get_geometric_mean(self) -> None:
        res = self.data_stats.get_geometric_mean(9)
        expected = 25.23013193
        self.assertAlmostEqual(expected, res)

    def test_get_mean_ad(self) -> None:
        res = self.data_stats.get_mean_ad(4)
        expected = 1.0
        self.assertAlmostEqual(expected, res)

    def test_get_median(self) -> None:
        res = self.data_stats.get_median(4)
        expected = 3.5
        self.assertAlmostEqual(expected, res)

    def test_get_median_ad(self) -> None:
        res = self.data_stats.get_median_ad(2)
        expected = 33.22
        self.assertAlmostEqual(expected, res)

    def test_get_num_nulls(self) -> None:
        res = self.data_stats.get_num_nulls(0)
        expected = 4
        self.assertEqual(expected, res)


if __name__ == "__main__":
    unittest.main()
