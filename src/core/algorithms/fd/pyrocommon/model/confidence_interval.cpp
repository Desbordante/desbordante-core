#include "core/algorithms/fd/pyrocommon/model/confidence_interval.h"

namespace model {

std::ostream& operator<<(std::ostream& ofs, ConfidenceInterval const& confidence_interval) {
    return ofs << static_cast<std::string>(confidence_interval);
}

}  // namespace model
