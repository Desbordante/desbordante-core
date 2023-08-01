#include "confidence_interval.h"

namespace structures {

std::ostream& operator<<(std::ostream& ofs, ConfidenceInterval const& confidence_interval) {
    return ofs << static_cast<std::string>(confidence_interval);
}

}  // namespace structures
