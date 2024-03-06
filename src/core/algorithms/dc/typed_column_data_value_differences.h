#include "table/typed_column_data.h"

namespace model {

/**
 * Computes the shared percentage of values between two columns of the same type.
 * The function is specialized for columns containing only int, double, or string
 * types. It calculates the frequency of each unique value in both columns and
 * determines the ratio of the shared values to the total values.
 *
 * Assumes that types of values in both columns are the same.
 *
 * @return A double representing the shared percentage of values between the two
 *         columns if the types are int, double, or string. Returns -1 (i.e this indicates
 *         that there are no similaties whatsoever) if the column types are not
 *         supported for comparison
 */
double GetSharedPercentage(TypedColumnData const& c1, TypedColumnData const& c2);

/**
 * Calculates the ratio of the smaller average to the larger average of two columns.
 *
 * Assumes that types of values in both columns are numeric and the same.
 *
 * @return A double representing the ratio of the smaller average to the larger average
 *         of the two columns if both columns are numeric. Returns -1 (i.e this indicates
 *         that there are no similaties whatsoever) if the column types are not
 *         types are not numeric.
 */
double GetAverageRatio(TypedColumnData const& c1, TypedColumnData const& c2);

}  // namespace model
