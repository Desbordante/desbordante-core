#pragma once

#include <filesystem>
#include <list>
#include <mutex>

#include <boost/any.hpp>

#include "algorithms/primitive.h"
#include "model/column_layout_typed_relation_data.h"
#include "model/fd.h"

namespace util {
class AgreeSetFactory;
}

namespace algos {

/* It is highly recommended to inherit your Algorithm from this class.
 * Consider TANE as an example of such a FDAlgorithm usage.
 * */
class FDAlgorithm : public CsvPrimitive {
private:
    friend util::AgreeSetFactory;

    std::mutex mutable register_mutex_;

    void RegisterOptions();

    void ResetState() final;
    virtual void ResetStateFd() = 0;

protected:
    size_t number_of_columns_;
    /* содержит множество найденных функциональных зависимостей. Это поле будет использоваться при
     * тестировании, поэтому важно положить сюда все намайненные ФЗ
     * */
    std::list<FD> fd_collection_;
    bool is_null_equal_null_;

    void FitInternal(model::IDatasetStream &data_stream) final;
    virtual void FitFd(model::IDatasetStream &data_stream) = 0;

public:
    constexpr static std::string_view kDefaultPhaseName = "FD mining";

    explicit FDAlgorithm(std::vector<std::string_view> phase_names);

    /* эти методы кладут зависимость в хранилище - можно пользоваться ими напрямую или
     * override-нуть, если нужно какое-то кастомное поведение
     * */
    virtual void RegisterFd(Vertical lhs, Column rhs) {
        std::scoped_lock lock(register_mutex_);
        fd_collection_.emplace_back(std::move(lhs), std::move(rhs));
    }
    virtual void RegisterFd(FD fd_to_register) {
        std::scoped_lock lock(register_mutex_);
        fd_collection_.push_back(std::move(fd_to_register));
    }

    /* fd_collection_ getter and setter */
    std::list<FD> const& FdList() const noexcept {
        return fd_collection_;
    }
    std::list<FD>& FdList() noexcept {
        return fd_collection_;
    }

    /* возвращает набор ФЗ в виде JSON-а. По сути, это просто представление фиксированного формата
     * для сравнения результатов разных алгоритмов. JSON - на всякий случай, если потом, например,
     * понадобится загрузить список в питон и как-нибудь его поанализировать
     * */
    std::string GetJsonFDs() const;

    /* Returns a vector of columns containing only unique values (i.e. keys).
     * Should be called after execute() only.
     * NOTE: retrieves keys from mined fds, so could be quite slow on wide
     * tables with many fds.
     * If your algorithm is inherited from FDAlgorithm but not from
     * PliBasedFDAlgorithm and generates ColumnLayoutRelationData from the
     * input table or in some similar way parses table, override this method
     * and use parsed table representation to retrieve keys (for performance
     * purposes).
     * PliBasedFDAlgorithm::GetKeys() is already overriding this method.
     */
    virtual std::vector<Column const*> GetKeys() const;

    // считает контрольную сумму Флетчера - нужно для тестирования по хешу
    unsigned int Fletcher16();

    virtual ~FDAlgorithm() = default;

    template <typename Container>
    static std::string FDsToJson(Container const& fds) {
        std::string result = "{\"fds\": [";
        std::vector<std::string> discovered_fd_strings;
        for (FD const& fd : fds) {
            discovered_fd_strings.push_back(fd.ToJSONString());
        }
        std::sort(discovered_fd_strings.begin(), discovered_fd_strings.end());
        for (std::string const& fd : discovered_fd_strings) {
            result += fd + ",";
        }
        if (result.back() == ',') {
            result.erase(result.size() - 1);
        }
        result += "]}";
        return result;
    }
};

}  // namespace algos
