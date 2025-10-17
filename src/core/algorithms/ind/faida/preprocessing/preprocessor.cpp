#include "preprocessor.h"

#include <cassert>

#include "algorithms/ind/faida/hashing/hashing.h"
#include "ind/faida/preprocessing/hashed_column_store.h"

namespace model {
class IDatasetStream;
}  // namespace model

namespace algos::faida {

std::unique_ptr<Preprocessor> Preprocessor::CreateHashedStores(
        std::string const& dataset_name,
        std::vector<std::shared_ptr<model::IDatasetStream>> const& data_streams, int sample_goal) {
    assert(!data_streams.empty());

    std::vector<std::unique_ptr<AbstractColumnStore>> stores;
    stores.reserve(data_streams.size());

    size_t const null_hash = hashing::CalcMurmurHash(std::string(""));

    TableIndex table_idx = 0;
    for (std::shared_ptr<model::IDatasetStream> const& input_data : data_streams) {
        auto store = HashedColumnStore::CreateFrom(dataset_name, table_idx++, *input_data,
                                                   sample_goal, null_hash);
        stores.emplace_back(std::move(store));
    }

    return std::make_unique<Preprocessor>(Preprocessor(std::move(stores), null_hash));
}

}  // namespace algos::faida
