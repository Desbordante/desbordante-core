#include "index_provider.h"

#include "dc/FastADC/model/predicate.h"

namespace algos::fastadc {

template <std::totally_ordered<> T>
size_t IndexProvider<T>::GetIndex(T object) {
    auto it = indexes_.find(object);
    if (it == indexes_.end()) {
        indexes_[object] = next_index_;
        objects_.push_back(object);
        return next_index_++;
    }
    return it->second;
}

template <std::totally_ordered<> T>
void IndexProvider<T>::AddAll(std::vector<T> const& objects) {
    for (auto object : objects) GetIndex(object);
}

template <std::totally_ordered<> T>
T IndexProvider<T>::GetObject(size_t index) const {
    return objects_.at(index);
}

template <std::totally_ordered<> T>
size_t IndexProvider<T>::Size() const {
    return objects_.size();
}

template <std::totally_ordered<> T>
void IndexProvider<T>::Sort() {
    std::sort(objects_.begin(), objects_.end());

    for (size_t i = 0; i < objects_.size(); ++i) indexes_[objects_[i]] = i;
}

template class IndexProvider<PredicatePtr>;

template class IndexProvider<int64_t>;
template class IndexProvider<double>;
template class IndexProvider<std::string>;
}  // namespace algos::fastadc
