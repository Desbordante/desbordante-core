#pragma once

#include <fstream>
#include <string>

template <typename T>
class Cursor {
    std::ifstream in_;
    T value_;

public:
    explicit Cursor(std::ifstream in) : in_(std::move(in)) {
        if (in_.fail()) {
            throw std::runtime_error("Received incorrect std::ifstream");
        }
        value_ = GetNext();
    }

    T const& GetValue() const {
        return value_;
    }
    T& GetValue() {
        return value_;
    }
    T const& GetNext() {
        std::getline(in_, GetValue());
        return GetValue();
    }
    bool HasNext() const {
        return !in_.eof();
    }
    void Print(std::ostream& out) const {
        out << GetValue() << " " << std::boolalpha << HasNext();
    }
};

using StrCursor = Cursor<std::string>;
