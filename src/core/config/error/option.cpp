#include "config/error/option.h"

#include "config/names_and_descriptions.h"

namespace config {
using names::kError, descriptions::kDError;
extern const CommonOption<ErrorType> ErrorOpt{
        kError, kDError, 0.0, {}, [](ErrorType error) {
            if (!(error >= 0 && error <= 1)) {
                throw std::invalid_argument("ERROR: error should be between 0 and 1.");
            }
        }};
}  // namespace config
