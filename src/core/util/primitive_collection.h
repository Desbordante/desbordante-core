#pragma once

#include <list>
#include <mutex>

namespace util {

/* Represents the collection of the primitive instances, guarantees the thread safety of adding
 * new instances
 */
template <typename T>
class PrimitiveCollection {
private:
    std::list<T> collection_;
    std::mutex mutable mutex_;

public:
    void Register(T primitive) {
        std::scoped_lock lock(mutex_);
        collection_.push_back(std::move(primitive));
    }

    template <typename... Args>
    void Register(Args&&... args) {
        std::scoped_lock lock(mutex_);
        collection_.emplace_back(std::forward<Args>(args)...);
    }

    void Clear() noexcept {
        std::scoped_lock lock(mutex_);
        collection_.clear();
    }

    size_t Size() const noexcept {
        std::scoped_lock lock(mutex_);
        return collection_.size();
    }

    /* Calling code MUST guarantee that methods below won't interfere with the registering of
     * new primitive instances or clearing (for the entire time the returned reference is held).
     * Practically this means that these methods should be called only after algorithm is finished
     * its execution.
     */
    std::list<T> const& AsList() const noexcept {
        return collection_;
    }

    std::list<T>& AsList() noexcept {
        return collection_;
    }
};

}  // namespace util
