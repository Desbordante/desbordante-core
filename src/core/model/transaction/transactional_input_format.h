#pragma once

#include <cstddef>

namespace model {

class InputFormat {
public:
    virtual size_t TidColumnIndex() const noexcept {
        return 0;
    };

    virtual size_t ItemColumnIndex() const noexcept {
        return 0;
    };

    virtual bool TidPresence() const noexcept = 0;

    virtual ~InputFormat() = default;
};

class Singular : public InputFormat {
private:
    size_t tid_column_index_;
    size_t item_column_index_;

public:
    explicit Singular(size_t tid_column_index, size_t item_column_index)
        : tid_column_index_(tid_column_index), item_column_index_(item_column_index) {}

    size_t TidColumnIndex() const noexcept override {
        return tid_column_index_;
    }

    size_t ItemColumnIndex() const noexcept override {
        return item_column_index_;
    }

    bool TidPresence() const noexcept override {
        return true;
    }
};

class Tabular : public InputFormat {
private:
    bool has_tid_;

public:
    explicit Tabular(bool has_tid) : has_tid_(has_tid) {}

    bool TidPresence() const noexcept override {
        return has_tid_;
    }
};

}  // namespace model
