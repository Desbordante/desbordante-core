#include <filesystem>
#include <memory>

#include <gtest/gtest.h>

#include "algorithms/functional/fd_algorithm.h"
#include "column_layout_typed_relation_data.h"
#include "csv_parser.h"
#include "datasets.h"

namespace tests {

namespace fs = std::filesystem;
namespace mo = model;

using mo::TypeId;
using algos::FDAlgorithm;

struct TypeParsingParams {
    std::vector<mo::TypeId> expected;
    std::string_view dataset;
    char const sep;
    bool const has_header;
    TypeParsingParams(std::vector<mo::TypeId> expected, std::string_view dataset,
                      char const sep = ',', bool const has_header = true) noexcept
        : expected(std::move(expected)), dataset(dataset), sep(sep), has_header(has_header) {}
};

class TestTypeParsing : public ::testing::TestWithParam<TypeParsingParams> {};

static fs::path ConstructPath(std::string_view dataset) {
    return test_data_dir / dataset;
}

TEST_P(TestTypeParsing, DefaultTest) {
    TypeParsingParams const& p = GetParam();
    CSVParser parser{ConstructPath(p.dataset), p.sep, p.has_header};
    std::vector<mo::TypedColumnData> column_data{mo::CreateTypedColumnData(parser, true)};

    ASSERT_EQ(column_data.size(), p.expected.size());

    size_t i = 0;
    for (mo::TypedColumnData const& col_data : column_data) {
        EXPECT_EQ(col_data.GetTypeId(), p.expected[i]) << "Column index: " << i;
        ++i;
    }
}

INSTANTIATE_TEST_SUITE_P(
    TypeSystem, TestTypeParsing,
    ::testing::Values(
        TypeParsingParams({TypeId::kString, TypeId::kMixed, TypeId::kMixed},
                          "WDC_appearances.csv"),
        TypeParsingParams({TypeId::kString, TypeId::kString, TypeId::kString},
                          "WDC_age.csv"),
        TypeParsingParams({TypeId::kString, TypeId::kMixed, TypeId::kMixed,
                           TypeId::kMixed},
                          "WDC_kepler.csv"),
        TypeParsingParams({TypeId::kString, TypeId::kString, TypeId::kMixed,
                           TypeId::kMixed, TypeId::kMixed, TypeId::kString,
                           TypeId::kString, TypeId::kString},
                          "WDC_satellites.csv"),
        TypeParsingParams({TypeId::kString, TypeId::kString, TypeId::kInt,
                           TypeId::kInt, TypeId::kInt, TypeId::kInt,
                           TypeId::kInt, TypeId::kUndefined, TypeId::kUndefined,
                           TypeId::kUndefined, TypeId::kUndefined, TypeId::kInt,
                           TypeId::kInt, TypeId::kInt, TypeId::kInt,
                           TypeId::kInt, TypeId::kUndefined, TypeId::kUndefined},
                          "CIPublicHighway700.csv"),
        TypeParsingParams({TypeId::kInt, TypeId::kInt, TypeId::kMixed,
                           TypeId::kInt, TypeId::kInt, TypeId::kInt,
                           TypeId::kInt},
                          "neighbors10k.csv"),
        TypeParsingParams({TypeId::kDouble, TypeId::kDouble, TypeId::kDouble,
                           TypeId::kDouble, TypeId::kString},
                          "iris.csv", ',', false),
        TypeParsingParams({TypeId::kUndefined,TypeId::kUndefined,TypeId::kUndefined,
                           TypeId::kInt, TypeId::kString, TypeId::kDouble,
                           TypeId::kBigInt, TypeId::kMixed, TypeId::kBigInt,
                           TypeId::kMixed, TypeId::kInt},
                          "SimpleTypes.csv")));

TEST(TypeSystem, SumColumnDoubles) {
    CSVParser parser{ConstructPath("iris.csv"), ',', false};
    std::vector<mo::TypedColumnData> col_data{mo::CreateTypedColumnData(parser, true)};
    ASSERT_EQ(col_data.size(), 5);
    mo::TypedColumnData const& col = col_data.front();
    ASSERT_EQ(col.GetTypeId(), static_cast<TypeId>(TypeId::kDouble));
    mo::INumericType const& type = static_cast<mo::INumericType const&>(col.GetType());
    std::unique_ptr<std::byte[]> sum(type.Allocate());
    for (std::byte const* value : col.GetData()) {
        type.Add(sum.get(), value, sum.get());
    }
    mo::Double expected = 876.5;
    EXPECT_DOUBLE_EQ(type.GetValue<mo::Double>(sum.get()), expected);
}

}  // namespace tests
