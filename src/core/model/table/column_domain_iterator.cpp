/** \file
 * \brief Column domain data iterator
 *
 * implementation of the column domain data iterator
 */
#include "column_domain_iterator.h"

#include <cassert>

namespace model {

std::vector<std::unique_ptr<ColumnDomainIterator::Reader>> ColumnDomainIterator::CreateReaders(
        ColumnDomain::RawData const& domain_data) {
    std::vector<std::unique_ptr<Reader>> readers;
    for (DomainPartition const& partition : domain_data) {
        if (!partition.IsNULL()) {
            readers.push_back(partition.GetReader());
        }
    }
    return readers;
}

ColumnDomainIterator::ReaderPQ ColumnDomainIterator::CreateReadersPQ(
        std::vector<std::unique_ptr<Reader>> const& readers) {
    ReaderPQ pq{ReaderGreater};
    for (auto const& reader : readers) {
        pq.push(reader.get());
    }
    return pq;
}

ColumnDomainIterator::ColumnDomainIterator(ColumnDomain const& domain)
    : domain_(domain),
      readers_(CreateReaders(domain_.get().GetData())),
      readers_pq_(CreateReadersPQ(readers_)) {
    MoveToNext();
}

void ColumnDomainIterator::MoveToNext() {
    assert(HasNext());
    Reader* reader = readers_pq_.top();
    value_ = reader->GetValue();
    do {
        readers_pq_.pop();
        if (reader->TryMove()) {
            readers_pq_.emplace(reader);
        }
        if (!HasNext()) break;
        reader = readers_pq_.top();
    } while (reader->GetValue() == value_);
}

}  // namespace model
