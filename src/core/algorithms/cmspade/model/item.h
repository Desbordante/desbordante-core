#pragma once
#include "types.h"

#include <string>
#include <cstddef>
#include <functional>
namespace algos::cmspade{
class Item{
private:
    Int id_;
public:
    Item(Int id) noexcept :id_(id){}

    Int GetId() const noexcept { return id_; }

    bool operator==(const Item &other) const { return id_ == other.id_; }
    bool operator<(const Item &other) const { return id_ < other.id_; }
    bool operator!=(const Item &other) const { return id_ != other.id_; }

};
} // namespace algos::cmspade
namespace std{
    template<> struct hash<algos::cmspade::Item>{
        std::size_t operator()(const algos::cmspade::Item& item) const noexcept{
            return std::hash<algos::cmspade::Int>()(item.GetId());
        }
    };
}