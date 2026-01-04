#pragma once

#include <cassert>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace model {
class ValueDictionary {
public:
    ValueDictionary() = default;

    bool Contains(std::string const& value) const {
        return to_int_map_.contains(value);
    }

    bool Contains(int value) const {
        return value > 0 && value < static_cast<int>(to_string_map_.size());
    }

    std::string const& ToString(int value) const {
        assert(Contains(value));
        return to_string_map_.at(value);
    }

    int ToInt(std::string const& new_value) {
        if (auto const& [it, success] =
                    to_int_map_.insert(std::make_pair(new_value, to_string_map_.size()));
            success) {
            to_string_map_.emplace_back(new_value);
            return to_string_map_.size() - 1;
        } else {
            return it->second;
        }
    }

private:
    std::vector<std::string> to_string_map_{""};
    std::unordered_map<std::string, int> to_int_map_;
};
}  // namespace model
