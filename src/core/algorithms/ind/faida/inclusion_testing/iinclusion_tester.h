#pragma once

#include <map>
#include <memory>
#include <unordered_set>

#include <boost/align/aligned_allocator.hpp>

#include "algorithms/ind/faida/preprocessing/abstract_column_store.h"
#include "algorithms/ind/faida/preprocessing/irow_iterator.h"
#include "algorithms/ind/faida/util/simple_cc.h"

namespace algos::faida {

class IInclusionTester {
public:
    using ActiveColumns = std::map<TableIndex, std::unordered_set<ColumnIndex>>;
    using HashedTableSample = AbstractColumnStore::HashedTableSample;

    virtual ActiveColumns SetCCs(std::vector<std::shared_ptr<SimpleCC>>& combinations) = 0;

    virtual void StartInsertRow(TableIndex table_idx) = 0;
    virtual void InsertRows(IRowIterator::Block const& hashed_rows, size_t block_size) = 0;

    virtual void FinalizeInsertion() = 0;
    virtual void Initialize(std::vector<HashedTableSample> const& samples) = 0;

    virtual bool IsIncludedIn(std::shared_ptr<SimpleCC> const& dep,
                              std::shared_ptr<SimpleCC> const& ref) = 0;

    virtual int GetNumCertainChecks() const = 0;
    virtual int GetNumUncertainChecks() const = 0;

    virtual ~IInclusionTester() = default;
};

}  // namespace algos::faida
