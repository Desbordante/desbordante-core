#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/model/types/type.h"

class Row {
private:
    std::unique_ptr<std::byte[]> buffer_;
    std::vector<std::byte const*> data_;
    size_t size_ = 0;

public:
    Row(std::vector<std::string> const& values,
        std::vector<std::shared_ptr<model::Type>> const& types, size_t row_size) {
        assert(values.size() == types.size());

        size_ = row_size;
        buffer_ = std::unique_ptr<std::byte[]>(new std::byte[size_]);
        data_ = std::vector<std::byte const*>(values.size(), nullptr);
        Update(values, types);
    }

    Row(std::unique_ptr<std::byte[]> buffer, std::vector<std::byte const*> data) noexcept
        : buffer_(std::move(buffer)), data_(std::move(data)) {}

    Row(Row const& other) {
        size_ = other.size_;
        buffer_ = std::make_unique<std::byte[]>(size_);
        std::copy(other.buffer_.get(), other.buffer_.get() + size_, buffer_.get());
        for (std::byte const* ptr : other.data_) {
            std::ptrdiff_t offset = ptr - other.buffer_.get();
            data_.push_back(buffer_.get() + offset);
        }
    }

    Row& operator=(Row const& other) = delete;
    Row(Row&& other) noexcept = default;
    Row& operator=(Row&& other) noexcept = default;

    ~Row() = default;

    void Update(std::vector<std::string> const& values,
                std::vector<std::shared_ptr<model::Type>> const& types) {
        assert(values.size() == types.size() and !data_.empty());

        size_t buf_index = 0;
        std::byte* buf = buffer_.get();
        for (size_t i = 0; i < values.size(); ++i) {
            std::shared_ptr<model::Type> const& cur_type = types[i];
            size_t const value_size = cur_type->GetSize();
            std::byte* next_val_buf = buf + buf_index;
            cur_type->ValueFromStr(next_val_buf, values[i]);
            data_[i] = next_val_buf;
            buf_index += value_size;
        }
    }

    std::vector<std::byte const*> const& GetData() const noexcept {
        return data_;
    }

    std::byte const* GetValue(size_t index) const noexcept {
        return data_[index];
    }

    size_t GetSize() const noexcept {
        return data_.size();
    }

    // std::string ToString() const final {
    //     return "Data for row " + row_->ToString();
    // }
};