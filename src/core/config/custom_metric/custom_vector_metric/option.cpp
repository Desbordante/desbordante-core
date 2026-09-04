#include "core/config/custom_metric/custom_vector_metric/option.h"

namespace config {

namespace {
void Normalize(CustomVectorMetricOptionType& value) {
    if (!value) {
        value = std::make_shared<util::DefaultCustomVectorMetric>();
    }
}
}  // namespace

Option<CustomVectorMetricOptionType> VectorMetricOption(CustomVectorMetricOptionType* value_ptr,
                                                        std::string_view name,
                                                        std::string_view description) {
    Option<CustomVectorMetricOptionType> option{value_ptr, name, description,
                                                CustomVectorMetricOptionType{nullptr}};
    option.SetNormalizeFunc(&Normalize);
    return option;
}

}  // namespace config
