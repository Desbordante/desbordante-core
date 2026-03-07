#pragma once

#include <filesystem>
#include <list>
#include <mutex>

#include <boost/any.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/fd/afd.h"
#include "core/config/max_lhs/type.h"
#include "core/util/primitive_collection.h"

namespace model {
class AgreeSetFactory;
}

namespace algos {

/* It is highly recommended to inherit your Algorithm from this class.
 * Consider TANE as an example of such a FDAlgorithm usage.
 * */
class AFDAlgorithm : public Algorithm {
private:
    friend model::AgreeSetFactory;

    void RegisterOptions();

    void ResetState() final;
    virtual void MakeExecuteOptsAvailableFDInternal() {};
    void MakeExecuteOptsAvailable() override;
    virtual void ResetStateFd() = 0;

protected:
    config::MaxLhsType max_lhs_;

    /* Collection of all discovered AFDs
     * Every AFD mining algorithm should place discovered dependecies here. Don't add new AFDs by
     * accessing this field directly, use RegisterAfd methods instead
     */
    util::PrimitiveCollection<AFD> afd_collection_;

    /* Registers new AFD.
     * Should be overrided if custom behavior is needed
     */
    virtual void RegisterAfd(Vertical lhs, Column rhs, long double threeshold,
                            std::shared_ptr<RelationalSchema const> const& schema) {
        if (lhs.GetArity() <= max_lhs_)
            afd_collection_.Register(std::move(lhs), std::move(rhs), std::move(threeshold), schema);
    }

    virtual void RegisterAfd(Vertical lhs, Column rhs,
                            std::shared_ptr<RelationalSchema const> const& schema) {
        if (lhs.GetArity() <= max_lhs_)
            afd_collection_.Register(std::move(lhs), std::move(rhs), 0, schema);
    }


    virtual void RegisterAfd(AFD afd_to_register) {
        if (afd_to_register.GetLhs().GetArity() <= max_lhs_)
            afd_collection_.Register(std::move(afd_to_register));
    }


    template <typename Container>
    static std::string AFDsToJson(Container const& afds) {
        std::string result = "{\"fds\": [";
        std::vector<std::string> discovered_fd_strings;
        for (AFD const& fd : afds) {
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

public:
    constexpr static std::string_view kDefaultPhaseName = "FD mining";

    explicit AFDAlgorithm(std::vector<std::string_view> phase_names);

    /* Returns the list of discovered FDs */
    std::list<AFD> const& AfdList() const noexcept {
        return afd_collection_.AsList();
    }

    std::list<AFD>& AfdList() noexcept {
        return afd_collection_.AsList();
    }

    std::list<AFD>& SortedAfdList();

    // считает контрольную сумму Флетчера - нужно для тестирования по хешу
    unsigned int Fletcher16();

    virtual ~AFDAlgorithm() = default;
};

}  // namespace algos
