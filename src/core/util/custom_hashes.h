#pragma once

#include "core/model/table/vertical.h"

namespace std {
template <>
struct hash<Vertical> {
    size_t operator()(Vertical const& k) const {
        return hash_value(k.GetColumnIndicesRef());
    }
};

template <>
struct hash<Column> {
    size_t operator()(Column const& k) const {
        return k.GetIndex();
    }
};

template <>
struct hash<std::shared_ptr<Vertical>> {
    size_t operator()(std::shared_ptr<Vertical> const& k) const {
        return std::hash<Vertical>()(*k);
    }
};

template <>
struct hash<std::shared_ptr<Column>> {
    size_t operator()(std::shared_ptr<Column> const& k) const {
        return std::hash<Column>()(*k);
    }
};

template <class T>
struct hash<std::pair<Vertical, T>> {
    size_t operator()(std::pair<Vertical, T> const& k) const {
        return std::hash<Vertical>()(k.first);
    }
};

template <class T>
struct hash<std::pair<std::shared_ptr<Vertical>, T>> {
    size_t operator()(std::pair<std::shared_ptr<Vertical>, T> const& k) const {
        return std::hash<Vertical>()(*k.first);
    }
};
}  // namespace std
