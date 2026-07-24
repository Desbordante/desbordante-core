#include "core/config/descriptions.h"

#include <algorithm>

#include "core/algorithms/cfd/enums.h"
#include "core/algorithms/cind/types.h"
#include "core/algorithms/fd/afd_metric/afd_metric.h"
#include "core/algorithms/fd/tane/enums.h"
#include "core/algorithms/md/hymd/enums.h"
#include "core/algorithms/metric/enums.h"
#include "core/algorithms/nar/des/enums.h"
#include "core/algorithms/od/fastod/od_ordering.h"
#include "core/config/enum_members_string.h"

namespace {
// Because all of the description definitions here are the same, using long descriptive names will
// IMO decrease readability. Under the assumption that these strings aren't going to change from
// "description" + "members", very short names are used here. If it doesn't hold up later, consider
// changing them.

// Workaround for passing a string literal as a template argument.
template <std::size_t N>
struct StringLiteralTemplateWrapper {
    char contents[N];

    explicit consteval StringLiteralTemplateWrapper(char const (&str)[N]) {
        std::copy_n(str, N, contents);
    }
};

template <std::size_t N>
using S = StringLiteralTemplateWrapper<N>;

template <StringLiteralTemplateWrapper S1, auto S2>
struct Concatenation {
    static constexpr std::size_t kLitLength = sizeof(S1.contents) - 1;
    static constexpr std::size_t kArrLength = S2.size() - 1;

    // Store as static class member to avoid lifetime issues. C++23 supports static template
    // function variables, would be better.
    static constexpr auto kArray = [] {
        std::array<char, kLitLength + kArrLength + 1> result;
        auto it = std::ranges::copy(S1.contents, result.begin()).out;
        std::ranges::copy(S2, --it);
        return result;
    }();

    static constexpr std::string_view kView{kArray.data(), kArray.size()};
};

// The concatenated string.
template <StringLiteralTemplateWrapper S1, auto S2>
constexpr std::string_view const& c = Concatenation<S1, S2>::kView;

template <typename EnumType>
constexpr auto const& m = config::kEnumMembersCStrBuffer<EnumType>;
}  // namespace

namespace config::descriptions {
std::string_view const kDMetric = c<S("metric to use\n"), m<algos::metric::Metric>>;
std::string_view const kDMetricAlgorithm =
        c<S("MFD algorithm to use\n"), m<algos::metric::MetricAlgo>>;
std::string_view const kDAFDMetric =
        c<S("AFD metric to calculate\n"), m<algos::afd_metric_calculator::AFDMetric>>;
std::string_view const kDCfdSubstrategy =
        c<S("CFD lattice traversal strategy to use\n"), m<algos::cfd::Substrategy>>;
std::string_view const kDPfdErrorMeasure =
        c<S("PFD error measure to use\n"), m<algos::PfdErrorMeasure>>;
std::string_view const kDAfdErrorMeasure =
        c<S("AFD error measure to use\n"), m<algos::AfdErrorMeasure>>;
std::string_view const kDLevelDefinition =
        c<S("MD lattice level definition to use\n"), m<algos::hymd::LevelDefinition>>;
std::string_view const kDDifferentialStrategy =
        c<S("DES mutation strategy to use\n"), m<algos::des::DifferentialStrategy>>;
std::string_view const kDODLeftOrdering =
        c<S("Ordering of the left attribute of OC or OD to use\n"), m<algos::od::Ordering>>;
std::string_view const kDConditionType =
        c<S("CIND condition types to use\n"), m<algos::cind::CondType>>;
std::string_view const kDAlgoType = c<S("CIND algorithm types to use\n"), m<algos::cind::AlgoType>>;
}  // namespace config::descriptions
