#pragma once

#include <stdexcept>

namespace config {

class ConfigurationError : public std::exception {
    std::string message_;

public:
    explicit ConfigurationError(std::string message) noexcept : message_(std::move(message)) {}

    const char* what() const noexcept override {
        return message_.data();
    }
};

class UnknownOption : public ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class TypeMismatch : public ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class NoDefault : public ConfigurationError {
    // Option does not have a default value, but value is not specified.
    using ConfigurationError::ConfigurationError;
};
class MustSpecifyExplicitly : public ConfigurationError {
    // Value has a default, but it cannot be obtained.
    using ConfigurationError::ConfigurationError;
};
class OutOfRange : public ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class InvalidChoice : public ConfigurationError {
    using ConfigurationError::ConfigurationError;
};
class EmptyCollection : public ConfigurationError {
    // Value is a collection that should not be empty but is.
    using ConfigurationError::ConfigurationError;
};

}  // namespace config
