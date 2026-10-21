#pragma once

#include <list>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/fd/afd.h"
#include "core/config/max_lhs/type.h"
#include "core/util/primitive_collection.h"

namespace algos {

class AFDAlgorithm : public Algorithm {
private:
    void RegisterOptions();

    void ResetState() final;
    virtual void MakeExecuteOptsAvailableFDInternal() {};
    void MakeExecuteOptsAvailable() override;
    virtual void ResetStateFd() = 0;

protected:
    config::MaxLhsType max_lhs_;

    /* Don't add new AFDs by accessing this field directly, use RegisterAfd methods instead */
    util::PrimitiveCollection<AFD> afd_collection_;

    void RegisterAfd(AFD afd_to_register) {
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
    explicit AFDAlgorithm();

    /* Returns the list of discovered FDs */
    std::list<AFD> const& AfdList() const noexcept {
        return afd_collection_.AsList();
    }

    std::list<AFD>& AfdList() noexcept {
        return afd_collection_.AsList();
    }

    std::list<AFD>& SortedAfdList();

    unsigned int Fletcher16();

    virtual ~AFDAlgorithm() = default;
};

}  // namespace algos
