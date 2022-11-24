#pragma once

#include "hashed_column_store.h"

namespace algos::faida {

class Preprocessor {
private:
    std::vector<std::unique_ptr<AbstractColumnStore>> stores_;

    size_t null_hash_;

    Preprocessor(std::vector<std::unique_ptr<AbstractColumnStore>>&& stores, size_t null_hash)
        : stores_(std::move(stores)), null_hash_(null_hash) {}

public:
    Preprocessor() = delete;
    Preprocessor(Preprocessor const& other) = delete;
    Preprocessor(Preprocessor&& other) = default;
    Preprocessor& operator=(Preprocessor const& other) = delete;
    Preprocessor& operator=(Preprocessor&& other) = delete;
    ~Preprocessor() = default;

    std::vector<std::unique_ptr<AbstractColumnStore>> const& GetStores() const {
        return stores_;
    }

    size_t GetNullHash() const {
        return null_hash_;
    }

    static std::unique_ptr<Preprocessor> CreateHashedStores(
            std::string const& dataset_name,
            std::vector<std::shared_ptr<model::IDatasetStream>> const& data_streams,
            int sample_goal);
};

}  // namespace algos::faida
