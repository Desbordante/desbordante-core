#pragma once

#include "core/model/table/idataset_stream.h"

namespace model {

namespace details {

namespace type_traits {
template <typename>
std::false_type IsPointerLike(unsigned long);

template <typename T>
auto IsPointerLike(int) -> decltype(*std::declval<T>(), std::true_type{});

template <typename T>
auto IsPointerLike(unsigned) -> decltype(std::declval<T>().get(), std::true_type{});

template <typename T>
concept is_ptr_like_v = decltype(IsPointerLike<T>(0))::value;
}  // namespace type_traits

///
/// \brief A dataset stream wrapper data, which allows store the stream as a pointer or by value.
///
/// \note The main goal is to be able to avoid unnecessary levels of indirection and virtual calls.
///       If possible, it is better to use a specific stream type and store it by value. Thanks to
///       this, we avoid unnecessary virtual calls and pointer calls.
///
/// \note This class should not be used directly except in the DatasetStreamWrapper class.
///
template <typename DatasetStream>
class DatasetStreamWrapperData {
private:
    using StreamField = std::conditional_t<type_traits::is_ptr_like_v<DatasetStream>,
                                           std::add_const_t<DatasetStream>, DatasetStream>;
    StreamField stream_;

public:
    template <typename Stream>
    explicit DatasetStreamWrapperData(Stream&& stream)
        requires type_traits::is_ptr_like_v<DatasetStream> ||
                 std::is_base_of_v<IDatasetStream, DatasetStream>
        : stream_(std::forward<Stream>(stream)) {}

    auto operator->() {
        if constexpr (type_traits::is_ptr_like_v<DatasetStream>) {
            if constexpr (std::is_pointer_v<DatasetStream>) {
                return stream_;
            } else {
                return stream_.get();
            }
        } else {
            return &stream_;
        }
    }

    auto operator->() const {
        if constexpr (type_traits::is_ptr_like_v<DatasetStream>) {
            if constexpr (std::is_pointer_v<DatasetStream>) {
                return stream_;
            } else {
                return stream_.get();
            }
        } else {
            return &stream_;
        }
    }
};
}  // namespace details

///
/// \brief A class that implements the IDatasetStream interface using the wrapper data class.
///
/// \note Classes that inherit from this class must implement only specific methods.
///       This is necessary in order to avoid code duplication.
///
template <typename DatasetStream>
class DatasetStreamWrapper : public IDatasetStream {
protected:
    details::DatasetStreamWrapperData<DatasetStream> stream_;

public:
    template <typename Stream>
    explicit DatasetStreamWrapper(Stream&& stream) : stream_(std::forward<Stream>(stream)) {}

    Row GetNextRow() override {
        return stream_->GetNextRow();
    }

    [[nodiscard]] bool HasNextRow() const override {
        return stream_->HasNextRow();
    }

    [[nodiscard]] size_t GetNumberOfColumns() const override {
        return stream_->GetNumberOfColumns();
    }

    [[nodiscard]] std::string GetColumnName(size_t index) const override {
        return stream_->GetColumnName(index);
    }

    [[nodiscard]] std::string GetRelationName() const override {
        return stream_->GetRelationName();
    }

    void Reset() override {
        stream_->Reset();
    }
};

}  // namespace model
