#include "transactional_data.h"

#include <cassert>
#include <stdexcept>
#include <unordered_map>

namespace model {

std::unique_ptr<TransactionalData> TransactionalData::CreateFrom(IDatasetStream& file_input,
                                                                 InputFormat const& input_type) {
    if (typeid(input_type) == typeid(Singular)) {
        return TransactionalData::CreateFromSingular(file_input, input_type.tid_column_index(),
                                                     input_type.item_column_index());
    } else if (typeid(input_type) == typeid(Tabular)) {
        return TransactionalData::CreateFromTabular(file_input, input_type.tid_presence());
    } else {
        throw std::logic_error("This input type is not maintained yet");
    }
}

std::unique_ptr<TransactionalData> TransactionalData::CreateFromSingular(IDatasetStream& file_input,
                                                                         unsigned tid_col_index,
                                                                         unsigned item_col_index) {
    std::vector<std::string> item_universe;
    std::unordered_map<std::string, unsigned> item_universe_set;
    std::unordered_map<unsigned, Itemset> transactions;
    unsigned latest_item_id = 0;

    assert(file_input.GetNumberOfColumns() >
           static_cast<int>(std::max(tid_col_index, item_col_index)));

    while (file_input.HasLines()) {
        std::vector<std::string> row = file_input.GetNextLine();
        if (row.empty()) {
            continue;
        }

        unsigned const tid = std::stoi(row[tid_col_index]);
        std::string& item_name = row[item_col_index];
        unsigned item_id = latest_item_id;

        auto const [item_iter, was_inserted] = item_universe_set.try_emplace(item_name, item_id);
        if (was_inserted) {
            // TODO(alexandrsmirn) попробовать избежать этого добавления.
            item_universe.push_back(std::move(item_name));
            ++latest_item_id;
        } else {
            // if this item already exists in the universe, set the old item id
            item_id = item_iter->second;
        }

        transactions[tid].AddItemId(item_id);
    }

    // sort items in each transaction
    for (auto& [tid, items] : transactions) {
        items.Sort();
    }

    return std::unique_ptr<TransactionalData>(new TransactionalData(std::move(item_universe),
                                                                    std::move(transactions)));
}

std::unique_ptr<TransactionalData> TransactionalData::CreateFromTabular(IDatasetStream& file_input,
                                                                        bool has_tid) {
    std::vector<std::string> item_universe;
    std::unordered_map<std::string, unsigned> item_universe_set;
    std::unordered_map<unsigned, Itemset> transactions;
    unsigned latest_item_id = 0;
    unsigned tid = 0;

    while (file_input.HasLines()) {
        std::vector<std::string> row = file_input.GetNextLine();
        if (row.empty()) {
            continue;
        }

        auto row_iter = row.begin();
        if (has_tid) {
            tid = std::stoi(*row_iter);
            row_iter++;
        }

        Itemset items;
        for (; row_iter != row.end(); ++row_iter) {
            std::string& item_name = *row_iter;
            if (item_name.empty()) {
                continue;
            }
            unsigned item_id = latest_item_id;

            auto const [item_iter, was_inserted] = item_universe_set.try_emplace(item_name, item_id);
            if (was_inserted) {
                // TODO(alexandrsmirn) попробовать избежать этого добавления.
                item_universe.push_back(std::move(item_name));
                ++latest_item_id;
            } else {
                // if this item already exists in the universe, set the old item id
                item_id = item_iter->second;
            }
            items.AddItemId(item_id);
        }

        items.Sort();
        transactions.emplace(tid, std::move(items));
        if (!has_tid) {
            ++tid;
        }
    }

    return std::unique_ptr<TransactionalData>(new TransactionalData(std::move(item_universe),
                                                                    std::move(transactions)));
}

} // namespace model
