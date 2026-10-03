#include <bitset>
#include <unordered_map>
#include <vector>

#include <boost/regex.hpp>

#include "core/model/types/type.h"

namespace model {

inline static std::vector<TypeId> const kAllCandidateTypes = {TypeId::kDate,   TypeId::kInt,
                                                              TypeId::kBigInt, TypeId::kDouble,
                                                              TypeId::kBool,   TypeId::kString};
inline static std::unordered_map<TypeId, boost::regex> const kTypeIdToRegex = {
        {TypeId::kDate,
         boost::regex(
                 R"(^(\d{4})([-.\/]?)(1[0-2]|0[1-9]|[1-9])\2(3[0-1]|0[1-9]|[1-9]|[1-2][0-9])$)")},
        {TypeId::kBool, boost::regex(R"(^\s*(true|false|0|1)\s*$)", boost::regex_constants::icase)},
        {TypeId::kDouble,
         boost::regex(
                 R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$|)"
                 R"(^[+-]?(?i)(inf|nan)(?-i)$|)"
                 R"(^[+-]?0[xX](((\d|[a-f]|[A-F]))+(\.(\d|[a-f]|[A-F])*)?|\.(\d|[a-f]|[A-F])+)([pP][+-]?\d+)?$)")},
        {TypeId::kBigInt, boost::regex(R"(^(\+|-)?\d{20,}$)")},
        {TypeId::kInt, boost::regex(R"(^(\+|-)?\d{1,19}$)")},
        {TypeId::kNull, boost::regex(Null::kValue.data())},
        {TypeId::kEmpty, boost::regex(R"(^$)")}};
inline static auto const kNullCheck = [](std::string const& val) {
    return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kNull));
};
inline static auto const kEmptyCheck = [](std::string const& val) {
    return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kEmpty));
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
inline static std::vector<std::pair<TypeId, std::function<bool(std::string const&)>>> const
        kTypeIdToChecker = {{TypeId::kDate,
                             [](std::string const& val) {
                                 return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kDate)) &&
                                        (kDelimitedDateCheck(val) || kUndelimitedDateCheck(val));
                             }},
                            {TypeId::kInt,
                             [](std::string const& val) {
                                 return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kInt));
                             }},
                            {TypeId::kBigInt,
                             [](std::string const& val) {
                                 return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kBigInt));
                             }},
                            {TypeId::kDouble,
                             [](std::string const& val) {
                                 return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kDouble));
                             }},
                            {TypeId::kBool, [](std::string const& val) {
                                 return boost::regex_match(val, kTypeIdToRegex.at(TypeId::kBool));
                             }}};
// each 1 represents a possible type from kAllCandidateTypes
inline static std::unordered_map<TypeId, std::bitset<6>> const kTypeIdToBitset = {
        {TypeId::kDate, std::bitset<6>("000001")},  // bitset for delimited dates
        {TypeId::kInt, std::bitset<6>("011110")},    {TypeId::kBigInt, std::bitset<6>("011100")},
        {TypeId::kDouble, std::bitset<6>("011000")}, {TypeId::kBool, std::bitset<6>("010000")},
        {TypeId::kString, std::bitset<6>("100000")}};

static TypeId DeduceColumnType(std::vector<std::string> const& col, bool treat_mixed_as_string) {
    bool is_undefined = true;
    std::bitset<6> candidate_types_bitset("111111");
    TypeId first_type_id = TypeId::kUndefined;
    auto matcher_map_iter = kTypeIdToChecker.begin();
    for (std::size_t i = 0; i != col.size(); ++i) {
        if (!kNullCheck(col[i]) && !kEmptyCheck(col[i])) {
            is_undefined = false;
            if (first_type_id != TypeId::kUndefined) {
                auto it =
                        std::find_if(matcher_map_iter, kTypeIdToChecker.end(),
                                     [&](auto const& pair) { return pair.first == first_type_id; });
                auto& type_check = it->second;
                if (type_check(col[i])) {
                    // undelimited and delimited dates have different bitsets
                    if (first_type_id == TypeId::kDate && kDelimitedDateCheck(col[i])) {
                        candidate_types_bitset &= kTypeIdToBitset.at(first_type_id);
                    }
                    continue;
                }
            }

            std::bitset<6> new_candidate_types_bitset("000000");
            bool matched = false;
            for (auto const& [type_id, type_check] : kTypeIdToChecker) {
                if (type_id != first_type_id && type_check(col[i])) {
                    if (first_type_id == TypeId::kUndefined && !matched) {
                        first_type_id = type_id;
                    }
                    matched = true;
                    new_candidate_types_bitset |= kTypeIdToBitset.at(type_id);
                    // possible value types are known at the first match except for dates
                    // (undelimited dates could be ints or doubles and delimited couldn't)
                    if (type_id == TypeId::kDate && kUndelimitedDateCheck(col[i])) {
                        new_candidate_types_bitset |= kTypeIdToBitset.at(TypeId::kInt);
                    }
                    break;
                }
            }
            if (!matched) {
                new_candidate_types_bitset = kTypeIdToBitset.at(TypeId::kString);
            }

            candidate_types_bitset &= new_candidate_types_bitset;
            if (candidate_types_bitset.none()) {
                if (treat_mixed_as_string) {
                    candidate_types_bitset = kTypeIdToBitset.at(TypeId::kString);
                } else {
                    return TypeId::kMixed;
                }
            }
        }
    }

    if (is_undefined) {
        return TypeId::kUndefined;
    }

    for (std::size_t i = 0; i < 6; i++) {
        if (candidate_types_bitset[i]) {
            return kAllCandidateTypes[i];
        }
    }

    return TypeId::kMixed;
}

}  // namespace model