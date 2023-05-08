#include "all_options.h"

#include <string>
#include <vector>

#include "algorithms/ar_algorithm_enums.h"
#include "algorithms/cfd/enums.h"
#include "algorithms/create_algorithm.h"
#include "algorithms/metric/enums.h"
#include "util/config/names_and_descriptions.h"

namespace algos {
template <typename T>
void validate(boost::any& v, const std::vector<std::string>& values, T*, int) {
    namespace po = boost::program_options;

    const std::string& s = po::validators::get_single_string(values);
    try {
        v = boost::any(T::_from_string_nocase(s.c_str()));
    } catch (std::runtime_error& e) {
        throw po::validation_error(po::validation_error::invalid_option_value);
    }
}
namespace metric {
using algos::validate;
}  // namespace metric

namespace cfd {
using algos::validate;
}  // namespace cfd
}  // namespace algos

namespace util::config {
boost::program_options::options_description GeneralOptions() {
    namespace po = boost::program_options;
    using namespace config::names;
    using namespace config::descriptions;

    std::string const kSeparatorOpt = std::string(kSeparator) + ",s";

    // clang-format off
    po::options_description general_options("General options");
    general_options.add_options()
            (kData, po::value<std::filesystem::path>()->required(), kDData)
            (kSeparatorOpt.c_str(), po::value<char>()->default_value(','), kDSeparator)
            (kHasHeader, po::value<bool>()->default_value(true), kDHasHeader)
            (kEqualNulls, po::value<bool>(), kDEqualNulls)
            (kThreads, po::value<ushort>(), kDThreads)
            ;
    // clang-format on
    return general_options;
}

boost::program_options::options_description AlgoOptions() {
    namespace po = boost::program_options;
    namespace desc = descriptions;

    // clang-format off
    po::options_description fd_options("FD options");
    fd_options.add_options()
            (names::kError, po::value<double>(), desc::kDError)
            (names::kMaximumLhs, po::value<unsigned int>(), desc::kDMaximumLhs)
            (names::kSeed, po::value<int>(), desc::kDSeed)
            ;

    po::options_description typo_options("Typo mining options");
    typo_options.add_options()
            (names::kRatio, po::value<double>(), desc::kDRatio)
            (names::kRadius, po::value<double>(), desc::kDRadius)
            (names::kApproximateAlgorithm, po::value<algos::AlgorithmType>(),
             desc::kDApproximateAlgorithm)
            (names::kPreciseAlgorithm, po::value<algos::AlgorithmType>(), desc::kDPreciseAlgorithm)
            ;

    po::options_description ar_options("AR options");
    ar_options.add_options()
            (names::kMinimumSupport, po::value<double>(), desc::kDMinimumSupport)
            (names::kMinimumConfidence, po::value<double>(), desc::kDMinimumConfidence)
            (names::kInputFormat, po::value<algos::InputFormat>(), desc::kDInputFormat)
            ;

    po::options_description ar_singular_options("AR \"singular\" input format options");
    ar_singular_options.add_options()
            (names::kTIdColumnIndex, po::value<unsigned>(), desc::kDTIdColumnIndex)
            (names::kItemColumnIndex, po::value<unsigned>(), desc::kDItemColumnIndex)
            ;

    po::options_description ar_tabular_options("AR \"tabular\" input format options");
    ar_tabular_options.add_options()
            (names::kFirstColumnTId, po::value<bool>()->default_value(true), desc::kDFirstColumnTId)
            ;

    ar_options.add(ar_singular_options).add(ar_tabular_options);

    po::options_description fd_verification_options("FD verification options");
    fd_verification_options.add_options()
            (names::kLhsIndices, po::value<std::vector<unsigned int>>()->multitoken(),
             desc::kDLhsIndices)
            (names::kRhsIndices, po::value<std::vector<unsigned int>>()->multitoken(),
             desc::kDRhsIndices)
            (names::kRhsIndex, po::value<unsigned int>(), desc::kDRhsIndex)
            ;

    po::options_description mfd_options("MFD options");
    mfd_options.add_options()
            (names::kMetric, po::value<algos::metric::Metric>(), desc::kDMetric)
            (names::kMetricAlgorithm, po::value<algos::metric::MetricAlgo>(), desc::kDMetricAlgorithm)
            (names::kParameter, po::value<long double>(), desc::kDParameter)
            (names::kDistFromNullIsInfinity, po::bool_switch(), desc::kDDistFromNullIsInfinity)
            ;

    po::options_description cosine_options("Cosine metric options");
    cosine_options.add_options()
            (names::kQGramLength, po::value<unsigned int>(), desc::kDQGramLength)
            ;

    mfd_options.add(cosine_options);

    po::options_description cfd_search_options("CFD mining options");
    cfd_search_options.add_options()
            (names::kCfdMinimumSupport, po::value<unsigned>(), desc::kDCfdMinimumSupport)
            (names::kCfdMinimumConfidence, po::value<double>(), desc::kDCfdMinimumConfidence)
            (names::kCfdMaximumLhs, po::value<unsigned>(), desc::kDCfdMaximumLhs)
            (names::kCfdColumnsNumber, po::value<unsigned>(), desc::kDCfdColumnsNumber)
            (names::kCfdTuplesNumber, po::value<unsigned>(), desc::kDCfdTuplesNumber)
            (names::kCfdSubstrategy, po::value<algos::cfd::Substrategy>(), desc::kDCfdSubstrategy)
            ;

    po::options_description ac_options("AC options");
    ac_options.add_options()
            (names::kBinaryOperation, po::value<algos::Binop>()->default_value(algos::Binop::Plus), desc::kDBinaryOperation)
            (names::kFuzziness, po::value<double>()->default_value(0.15), desc::kDFuzziness)
            (names::kFuzzinessProbability, po::value<double>()->default_value(0.9), desc::kDFuzzinessProbability)
            (names::kWeight, po::value<double>()->default_value(0.05), desc::kDWeight)
            (names::kBumpsLimit, po::value<size_t>()->default_value(5), desc::kDBumpsLimit)
            (names::kIterationsLimit, po::value<size_t>()->default_value(10), desc::kDIterationsLimit)
            (names::kACSeed, po::value<double>()->default_value(0.0), desc::kDACSeed)
            ;
    // clang-format on

    po::options_description algorithm_options("Algorithm options");
    algorithm_options.add(fd_options)
            .add(mfd_options)
            .add(ar_options)
            .add(ac_options)
            .add(typo_options)
            .add(fd_verification_options)
            .add(cfd_search_options);
    return algorithm_options;
}
}  // namespace util::config
