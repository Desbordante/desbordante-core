#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/algorithms/fd/fdhits/refinement_helper.h"
#include "core/algorithms/fd/fdhits/types.h"
#include "core/algorithms/fd/hycommon/types.h"

namespace algos::fd::fdhits {

class PLITable {
private:
    hy::PLIsPtr plis_;
    std::shared_ptr<hy::Columns> inverse_;
    hy::RowsPtr compressed_records_;

    RowIndex row_count_;
    Pli full_pli_;
    std::optional<std::string> name_;

    static thread_local RefinementHelper refinement_helper_;

    static void SortPliBy(Pli& pli, std::vector<ClusterIndex> const& target);

public:
    PLITable(hy::PLIsPtr plis, std::shared_ptr<hy::Columns> inverse, hy::RowsPtr records,
             std::optional<std::string> name = std::nullopt);

    std::optional<std::string> const& GetName() const {
        return name_;
    }

    std::size_t GetColumnCount() const {
        return plis_->size();
    }

    RowIndex GetRowCount() const {
        return row_count_;
    }

    Pli const& GetColumnPli(std::size_t column) const {
        return plis_->at(column)->GetIndex();
    }

    Pli const& GetFullPli() const {
        return full_pli_;
    }

    bool IsDifferentOnColumn(RowIndex r1, RowIndex r2, std::size_t column) const {
        return (*inverse_)[column][r1] == 0 || (*inverse_)[column][r1] != (*inverse_)[column][r2];
    }

    void SortPlisBy(std::size_t target);

    void Intersect(Pli const& pli, bool first, std::size_t column, ClusterFilter& filter,
                   Pli& intersection) const;

    hy::Columns const& GetInverse() const {
        return *inverse_;
    }

    hy::Rows const& GetCompressedRecords() const {
        return *compressed_records_;
    }
};

}  // namespace algos::fd::fdhits
