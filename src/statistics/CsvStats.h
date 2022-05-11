#include "CSVParser.h"
#include "ColumnLayoutTypedRelationData.h"
#include "NumericType.h"
 
namespace fs = std::filesystem;
namespace mo = model;

class CsvStats {
private:
    /*const std::vector<mo::TypedColumnData>
        col_data_;*/  // class have a vector to avoid a lot of requests
                    // we dont want to change data accidently thats why vector is const

public:
    const std::vector<mo::TypedColumnData>
        col_data_;

    CsvStats(std::string_view data, char sep, bool has_header);

    std::byte* GetSum(int index);
    int Count(int index);
    std::byte* GetAvg(int index);
    std::byte* GetSTD(int index);
    std::byte* CountStndrtCentrlMomntOfDist(int index, int number);
    std::byte* GetSkewness(int index);
    std::byte* GetKurtosis(int index);
    std::byte* CountCentrlMomntOfDist(int index, int number);
    /*std::optional<std::variant<mo::Int, mo::Double>> GetSum(int index);*/
};