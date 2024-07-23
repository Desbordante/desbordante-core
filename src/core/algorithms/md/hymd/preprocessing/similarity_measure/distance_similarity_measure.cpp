#include "algorithms/md/hymd/preprocessing/similarity_measure/distance_similarity_measure.h"

#include "algorithms/md/hymd/preprocessing/similarity_measure/value_processing_worker.h"

namespace algos::hymd::preprocessing::similarity_measure {

template <typename IndexType>
class DistanceValueProcessingWorker : public ValueProcessingWorker<IndexType> {
    using Base = ValueProcessingWorker<IndexType>;

    std::function<double(std::byte const*, std::byte const*)> compute_distance_;
    Similarity const min_sim_;

public:
    DistanceValueProcessingWorker(
            std::shared_ptr<DataInfo const> const& data_info_left,
            std::shared_ptr<DataInfo const> const& data_info_right,
            std::vector<indexes::PliCluster> const& clusters_right,
            ValidTableResults<Similarity>& task_data,
            std::function<double(std::byte const*, std::byte const*)> compute_distance,
            Similarity min_sim)
        : Base(std::move(data_info_left), std::move(data_info_right), clusters_right, task_data),
          compute_distance_(compute_distance),
          min_sim_(min_sim) {}

    bool CalcAndAdd(ValueIdentifier left_value_id, RowInfo<Similarity>& row_info,
                    ValueIdentifier start_from) override {
        bool dissimilar_found_here = false;
        double max_distance = 0;
        std::vector<double> distances(Base::data_right_size_);
        std::byte const* left_value = Base::data_info_left_->GetAt(left_value_id);
        for (ValueIdentifier right_index = start_from; right_index != Base::data_right_size_;
             ++right_index) {
            double distance =
                    compute_distance_(left_value, Base::data_info_right_->GetAt(right_index));
            distances[right_index] = distance;
            if (distance > max_distance) {
                max_distance = distance;
            }
        }
        for (ValueIdentifier right_index = start_from; right_index != Base::data_right_size_;
             ++right_index) {
            double normalized_similarity =
                    max_distance > 0 ? 1.0 - (distances[right_index] / max_distance) : 1.0;
            if (normalized_similarity < min_sim_) {
                dissimilar_found_here = true;
                continue;
            }
            Base::AddValue(row_info, right_index, normalized_similarity);
        }
        return dissimilar_found_here;
    }
};

indexes::SimilarityMeasureOutput DistanceSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const& clusters_right) const {
    return MakeIndexesTemplate<DistanceValueProcessingWorker>(data_info_left, data_info_right,
                                                              clusters_right, pool_, picker_,
                                                              compute_distance_, min_sim_);
}
}  // namespace algos::hymd::preprocessing::similarity_measure
