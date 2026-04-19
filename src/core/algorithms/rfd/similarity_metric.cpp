#include "core/algorithms/rfd/similarity_metric.h"

#include <algorithm>
#include <vector>

namespace algos::rfd {

FunctionSimilarityMetric::FunctionSimilarityMetric(Func func) : func_(std::move(func)) {}

double FunctionSimilarityMetric::Compare(const std::string& a, const std::string& b) const {
    return func_(a, b);
}

namespace {
double LevenshteinSimilarity(const std::string& a, const std::string& b) {
    size_t n = a.size(), m = b.size();
    if (n == 0 && m == 0) return 1.0;
    if (n == 0 || m == 0) return 0.0;
    std::vector<size_t> dp(m + 1);
    for (size_t j = 0; j <= m; ++j) dp[j] = j;
    for (size_t i = 1; i <= n; ++i) {
        size_t prev = i;
        for (size_t j = 1; j <= m; ++j) {
            size_t cost = (a[i-1] == b[j-1]) ? 0 : 1;
            size_t cur = std::min({dp[j] + 1, prev + 1, dp[j-1] + cost});
            dp[j-1] = prev;
            prev = cur;
        }
        dp[m] = prev;
    }
    double max_len = std::max(n, m);
    return 1.0 - static_cast<double>(dp[m]) / max_len;
}
} // namespace

std::shared_ptr<SimilarityMetric> LevenshteinMetric() {
    return std::make_shared<FunctionSimilarityMetric>(&LevenshteinSimilarity);
}

std::shared_ptr<SimilarityMetric> EqualityMetric() {
    return std::make_shared<FunctionSimilarityMetric>(
        [](const std::string& a, const std::string& b) { return a == b ? 1.0 : 0.0; });
}

} // namespace algos::rfd
