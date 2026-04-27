#pragma once
#include "Prefix.hpp"
#include "Bitmap.hpp"
#include <memory>

class PatternTKS {
public:
    Prefix prefix;
    int support;
    std::unique_ptr<Bitmap> bitmap; 

    PatternTKS(Prefix prefix, int support)
        : prefix(std::move(prefix)), support(support), bitmap(nullptr) {}

    PatternTKS(PatternTKS&& other) noexcept
        : prefix(std::move(other.prefix)), 
          support(other.support), 
          bitmap(std::move(other.bitmap)) {}

    PatternTKS(const PatternTKS& other)
        : prefix(other.prefix), 
          support(other.support), 
          bitmap(other.bitmap ? std::make_unique<Bitmap>(*other.bitmap) : nullptr) {}

    PatternTKS& operator=(PatternTKS&& other) noexcept {
        if (this != &other) {
            prefix = std::move(other.prefix);
            support = other.support;
            bitmap = std::move(other.bitmap);
        }
        return *this;
    }

    PatternTKS& operator=(const PatternTKS& other) {
        if (this != &other) {
            prefix = other.prefix;
            support = other.support;
            bitmap = other.bitmap ? std::make_unique<Bitmap>(*other.bitmap) : nullptr;
        }
        return *this;
    }

    bool operator<(const PatternTKS& other) const {
        if (&other == this) return false;
        int compare = this->support - other.support;
        if (compare != 0) return compare > 0;
        size_t hashThis = reinterpret_cast<size_t>(this);
        size_t hashOther = reinterpret_cast<size_t>(&other);
        return hashThis < hashOther;
    }
};