#include "core/config/custom_metric/custom_vector_metric/option.h"

namespace config {

namespace {
void Normalize(CustomVectorMetricType& value) {
    if (!value) {
        value = std::make_shared<util::DefaultCustomVectorMetric>();
    }
}
}  // namespace

Option<CustomVectorMetricType> VectorMetricOption(CustomVectorMetricType* value_ptr,
                                                  std::string_view name,
                                                  std::string_view description) {
    Option<CustomVectorMetricType> option{value_ptr, name, description,
                                          CustomVectorMetricType{nullptr}};
    option.SetNormalizeFunc(&Normalize);
    return option;
}

}  // namespace config
