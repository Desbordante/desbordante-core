#include "algorithms/md/hymd/preprocessing/data_info.h"

#include <cstddef>

namespace algos::hymd::preprocessing {

auto DataInfo::MakeDataPtr(std::unique_ptr<std::byte[]> data, model::Type::Destructor destructor,
                           std::size_t const elements, std::size_t const type_size)
        -> std::unique_ptr<std::byte[], DataDeleter> {
    DataDeleter deleter = [destructor = std::move(destructor), elements,
                           type_size](std::byte* data) {
        if (destructor) {
            for (ValueIdentifier value_id = 0; value_id < elements; ++value_id) {
                destructor(data + value_id * type_size);
            }
        }
        delete[] data;
    };
    return {data.release(), std::move(deleter)};
}

DataInfo::DataInfo(std::size_t elements, std::size_t type_size, model::Type::Destructor destructor,
                   std::unique_ptr<std::byte[]> data)
    : elements_(elements),
      type_size_(type_size),
      data_(MakeDataPtr(std::move(data), std::move(destructor), elements_, type_size_)) {}

std::shared_ptr<DataInfo> DataInfo::MakeFrom(indexes::KeyedPositionListIndex const& pli,
                                             model::Type const& type) {
    std::size_t const value_number = pli.GetClusters().size();
    std::size_t const type_size = type.GetSize();
    auto data = std::unique_ptr<std::byte[]>(type.Allocate(value_number));
    for (auto const& [string, value_id] : pli.GetMapping()) {
        type.ValueFromStr(&data[value_id * type_size], string);
    }
    return std::make_shared<DataInfo>(value_number, type_size, type.GetDestructor(),
                                      std::move(data));
}

}  // namespace algos::hymd::preprocessing
