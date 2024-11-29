#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/nar/des/des.h"
#include "all_csv_configs.h"
#include "config/names.h"

namespace tests {

class DESTest : public ::testing::Test {
protected:
    static algos::StdParamsMap GetParamMap(CSVConfig const& csv_config, double minsup,
                                           double minconf, unsigned int popSize, unsigned int evalNum,
                                           double crossProb, double diffScale,
                                           algos::des::DifferentialStrategy diffStrategy) {
        using namespace config::names;
        return {{kCsvConfig, csv_config},          {kMinimumSupport, minsup},
                {kMinimumConfidence, minconf},     {kPopulationSize, popSize},
                {kMaxFitnessEvaluations, evalNum}, {kCrossoverProbability, crossProb},
                {kDifferentialScale, diffScale},   {kDifferentialStrategy, diffStrategy}};
    }

    template <typename... Args>
    static std::unique_ptr<algos::des::DES> CreateAlgorithmInstance(Args&&... args) {
        return algos::CreateAndLoadAlgorithm<algos::des::DES>(
                GetParamMap(std::forward<Args>(args)...));
    }
};

TEST_F(DESTest, LaunchTest) {
    auto algorithm = CreateAlgorithmInstance(kAbalone, 0.5, 0.6, 100u, 100u, 0.9, 0.5,
                                             algos::des::DifferentialStrategy::rand1Bin);
    algorithm->Execute();
    unsigned int counter = 0;
    std::string result;
    for(auto i: algorithm->GetNARVector()) {
        result += "\n";
        result += std::to_string(counter);
        result +=  ": ";
        result +=  i.ToString();
        counter++;
    }
    // TEMPORARY
    std::string expected = std::string("\n0: 0.614583 {2: [0.246137 - 0.449296], 8: [2 - 21]} ===> {7: [0.025494 - 0.998732]} s: 0.510414 c: 1.000000") +
        std::string("\n1: 0.592672 {0: [I], 1: [0.262000 - 0.815000], 5: [0.535755 - 0.586535], 6: [0.272535 - 0.496650], 7: [0.233198 - 0.347366], 8: [7 - 27]} ===> {4: [0.002000 - 2.825500]} s: 0.000239 c: 1.000000") +
        std::string("\n2: 0.556569 {7: [0.286728 - 0.864815], 8: [9 - 29]} ===> {2: [0.055000 - 0.643457]} s: 0.337084 c: 0.999290") +
        std::string("\n3: 0.555795 {0: [M], 5: [0.724435 - 0.842897], 6: [0.420764 - 0.448567], 7: [0.002343 - 0.694841], 8: [1 - 21]} ===> {3: [0.000000 - 0.304527]} s: 0.000718 c: 1.000000") +
        std::string("\n4: 0.493177 {0: [M]} ===> {2: [0.164961 - 0.616953], 4: [0.558857 - 2.019646], 6: [0.073513 - 0.389915]} s: 0.277232 c: 0.757853") +
        std::string("\n5: 0.468389 {1: [0.247455 - 0.807493], 2: [0.154824 - 0.596962], 6: [0.244216 - 0.557378], 7: [0.203497 - 0.436829], 8: [12 - 29]} ===> {5: [0.468622 - 0.875126]} s: 0.035672 c: 0.702830") +
        std::string("\n6: 0.436493 {1: [0.075000 - 0.334574], 4: [0.133621 - 0.229566]} ===> {3: [0.061175 - 0.382962]} s: 0.020589 c: 0.955556") +
        std::string("\n7: 0.408549 {3: [0.105373 - 0.666329], 6: [0.421473 - 0.505438], 8: [9 - 15]} ===> {1: [0.217459 - 0.730138]} s: 0.011970 c: 0.769231") +
        std::string("\n8: 0.393278 {0: [F], 4: [1.109309 - 2.825500]} ===> {2: [0.195649 - 0.538823]} s: 0.100072 c: 0.746429") +
        std::string("\n9: 0.330923 {2: [0.283966 - 0.650000], 6: [0.258812 - 0.401132], 7: [0.011526 - 0.851961], 8: [11 - 25]} ===> {5: [0.628561 - 1.376670]} s: 0.044051 c: 0.393162") +
        std::string("\n10: 0.245074 {0: [M]} ===> {1: [0.472117 - 0.682620], 4: [0.846834 - 2.493609], 7: [0.073271 - 0.705797], 8: [13 - 28]} s: 0.048121 c: 0.131545") +
        std::string("\n11: 0.240821 {5: [0.678022 - 0.688435]} ===> {0: [F], 4: [1.044809 - 2.228662], 6: [0.302744 - 0.643207], 7: [0.546592 - 0.972107], 8: [1 - 18]} s: 0.000239 c: 0.055556") +
        std::string("\n12: 0.211013 {8: [13 - 13]} ===> {0: [M], 2: [0.083403 - 0.608382], 5: [0.659852 - 1.262961], 6: [0.314217 - 0.592022]} s: 0.003591 c: 0.073892") +
        std::string("\n13: 0.202699 {4: [1.295915 - 2.825500], 6: [0.267328 - 0.760000]} ===> {0: [M], 1: [0.254730 - 0.799600], 8: [13 - 13]} s: 0.006943 c: 0.045597") +
        std::string("\n14: 0.187660 {4: [1.598772 - 2.825500]} ===> {0: [M], 1: [0.654532 - 0.719016], 3: [0.093262 - 0.351710], 8: [22 - 26]} s: 0.000479 c: 0.006944") +
        std::string("\n15: 0.185542 {0: [I], 6: [0.007141 - 0.186399]} ===> {2: [0.300093 - 0.341638], 4: [0.761850 - 1.635220], 7: [0.001500 - 0.266909]} s: 0.000239 c: 0.000831") +
        std::string("\n16: 0.182276 {1: [0.508862 - 0.741533], 7: [0.204796 - 0.251499]} ===> {0: [F], 6: [0.200488 - 0.714955]} s: 0.009576 c: 0.092807") +
        std::string("\n17: 0.149640 {0: [I], 1: [0.273092 - 0.700549], 4: [0.690238 - 1.640147]} ===> {8: [21 - 27]} s: 0.000239 c: 0.004237") +
        std::string("\n18: 0.148624 {4: [0.793846 - 2.592043]} ===> {1: [0.128696 - 0.657298], 2: [0.431974 - 0.452565], 7: [0.424356 - 0.560183]} s: 0.000479 c: 0.000950") +
        std::string("\n19: 0.113370 {3: [0.202634 - 0.519985]} ===> {0: [I], 2: [0.494097 - 0.544377]} s: 0.000239 c: 0.006536") +
        std::string("\n20: 0.093495 {0: [F]} ===> {5: [0.705051 - 0.785856]} s: 0.013886 c: 0.044376");
    
    if(result == expected) {
    SUCCEED();
    } else {
        FAIL();
    }
}

}  // namespace tests
