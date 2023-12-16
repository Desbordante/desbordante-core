#pragma once

#include <stdexcept>

namespace config {

class ConfigurationError : public std::exception {
    std::string message_;

public:
    explicit ConfigurationError(std::string message) noexcept : message_(std::move(message)) {}

    char const* what() const noexcept override {
        return message_.data();
    }
};

}  // namespace config
