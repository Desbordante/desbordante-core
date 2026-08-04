#include <bitset>
#include <unordered_map>
#include <vector>

#include <boost/regex.hpp>

#include "core/model/types/type.h"

namespace model {

inline static std::vector<TypeId> const kAllCandidateTypes = {
        +TypeId::kDate, +TypeId::kInt, +TypeId::kBigInt, +TypeId::kDouble, +TypeId::kString};
inline static std::unordered_map<TypeId, boost::regex> const kTypeIdToRegex = {
        {TypeId::kDate,
         boost::regex(
                 R"(^(\d{4})([-.\/]?)(1[0-2]|0[1-9]|[1-9])\2(3[0-1]|0[1-9]|[1-9]|[1-2][0-9])$)")},
        {TypeId::kDouble,
         boost::regex(
                 R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
                 R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
                 R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)")},
        {+TypeId::kBigInt, boost::regex(R"(^(\+|-)?\d{20,}$)")},
        {+TypeId::kInt, boost::regex(R"(^(\+|-)?\d{1,19}$)")},
        {+TypeId::kNull, boost::regex(Null::kValue.data())},
        {+TypeId::kEmpty, boost::regex(R"(^$)")}};
inline static auto const kNullCheck = [](std::string const& val) {
    return boost::regex_match(val, kTypeIdToRegex.at(+TypeId::kNull));
};
inline static auto const kEmptyCheck = [](std::string const& val) {
    return boost::regex_match(val, kTypeIdToRegex.at(+TypeId::kEmpty));
};
inline static std::function<bool(std::string const&)> const kUndelimitedDateCheck =
        [](std::string const& val) {
            bool is_undelimited_date = false;
            try {
                boost::gregorian::from_undelimited_string(val);
                is_undelimited_date = true;
            } catch (...) {
            }
            return is_undelimited_date;
        };
inline static std::function<bool(std::string const&)> const kDelimitedDateCheck =
        [](std::string const& val) {
            bool is_simple_date = false;
            try {
                boost::gregorian::from_simple_string(val);
                is_simple_date = true;
            } catch (...) {
            }
            return is_simple_date;
        };
inline static std::unordered_map<TypeId, std::function<bool(std::string const&)>> const
        kTypeIdToChecker = {
                {TypeId::kDouble,
                 [](std::string const& val) {
                     return boost::regex_match(val, kTypeIdToRegex.at(+TypeId::kDouble));
                 }},
                {TypeId::kBigInt,
                 [](std::string const& val) {
                     return boost::regex_match(val, kTypeIdToRegex.at(+TypeId::kBigInt));
                 }},
                {TypeId::kInt,
                 [](std::string const& val) {
                     return boost::regex_match(val, kTypeIdToRegex.at(+TypeId::kInt));
                 }},
                {TypeId::kDate, [](std::string const& val) {
                     return boost::regex_match(val, kTypeIdToRegex.at(+TypeId::kDate)) &&
                            (kDelimitedDateCheck(val) || kUndelimitedDateCheck(val));
                 }}};
// each 1 represents a possible type from kAllCandidateTypes
inline static std::unordered_map<TypeId, std::bitset<5>> const kTypeIdToBitset = {
        {+TypeId::kDate, std::bitset<5>("00001")},  // bitset for delimited dates
        {+TypeId::kInt, std::bitset<5>("01110")},
        {+TypeId::kBigInt, std::bitset<5>("01100")},
        {+TypeId::kDouble, std::bitset<5>("01000")},
        {+TypeId::kString, std::bitset<5>("10000")}};

/**
 * @brief Deduces the data type of a column based on its unparsed string values.
 *
 * This function analyzes all values in the column to determine their common type.
 * It uses a bitset-based filtering approach to progressively narrow down candidate types
 * as each value is examined.
 *
 * The algorithm works as follows:
 * 1. Iterates through all unparsed values in the column, skipping null and empty values.
 * 2. For the first non-null/non-empty value, determines the initial candidate type.
 * 3. For subsequent values:
 *    - If the value matches the first type's checker, it's accepted (with special handling for
 * dates).
 *    - Otherwise, finds all matching types and builds a bitset of candidates.
 *    - Special case: undelimited dates can also be integers or doubles.
 * 4. Intersects the candidate types bitset with the new candidates using bitwise AND.
 * 5. If no candidates remain, either defaults to String (if treat_mixed_as_string_ is true)
 *    or returns Mixed type.
 *
 * Special handling:
 * - Dates are treated specially as undelimited dates have different type possibilities
 *   than delimited dates.
 * - Mixed types are returned when values cannot be reconciled to a single type
 *   (unless treat_mixed_as_string_ is enabled).
 *
 * @return TypeId The deduced type of the column (kUndefined, kString, kInt, kDouble,
 *                kDate, or kMixed).
 */
static TypeId DeduceColumnType(std::vector<std::string> const& col, bool treat_mixed_as_string_) {
    bool is_undefined = true;
    std::bitset<5> candidate_types_bitset("11111");
    TypeId first_type_id = +TypeId::kUndefined;
    for (std::size_t i = 0; i != col.size(); ++i) {
        std::string const& cur_val = col[i];
        if (!kNullCheck(cur_val) && !kEmptyCheck(cur_val)) {
            is_undefined = false;
            if (first_type_id != +TypeId::kUndefined) {
                auto& type_check = kTypeIdToChecker.at(first_type_id);
                if (type_check(cur_val)) {
                    // undelimited and delimited dates have different bitsets
                    if (first_type_id == +TypeId::kDate && kDelimitedDateCheck(cur_val)) {
                        candidate_types_bitset &= kTypeIdToBitset.at(first_type_id);
                    }
                    continue;
                }
            }

            std::bitset<5> new_candidate_types_bitset("00000");
            bool matched = false;
            for (auto const& [type_id, type_check] : kTypeIdToChecker) {
                if (type_id != first_type_id && type_check(cur_val)) {
                    if (first_type_id == +TypeId::kUndefined && !matched) {
                        first_type_id = type_id;
                    }
                    matched = true;
                    new_candidate_types_bitset |= kTypeIdToBitset.at(type_id);
                    // possible value types are known at the first match except for dates
                    // (undelimited dates could be ints or doubles and delimited couldn't)
                    if (type_id == +TypeId::kDate && kUndelimitedDateCheck(cur_val)) {
                        new_candidate_types_bitset |= kTypeIdToBitset.at(+TypeId::kInt);
                    }
                    break;
                }
            }
            if (!matched) {
                new_candidate_types_bitset = kTypeIdToBitset.at(+TypeId::kString);
            }

            candidate_types_bitset &= new_candidate_types_bitset;
            if (candidate_types_bitset.none()) {
                if (treat_mixed_as_string_) {
                    candidate_types_bitset = kTypeIdToBitset.at(+TypeId::kString);
                } else {
                    return +TypeId::kMixed;
                }
            }
        }
    }

    if (is_undefined) {
        return +TypeId::kUndefined;
    }

    for (std::size_t i = 0; i < 5; i++) {
        if (candidate_types_bitset[i]) {
            return kAllCandidateTypes[i];
        }
    }

    return +TypeId::kMixed;
}

}  // namespace model