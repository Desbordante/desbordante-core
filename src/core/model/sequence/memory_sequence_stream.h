#pragma once

#include <string>
#include <vector>

namespace model {

struct SequenceRecord {
    std::vector<std::string> events;
    size_t timestamp;
};

class MemorySequenceStream {
private:
    std::vector<SequenceRecord> data_;
    size_t current_entry_ = 0;

public:
    MemorySequenceStream(std::vector<SequenceRecord> data) : data_(std::move(data)) {}

    SequenceRecord GetNext() {
        return data_[current_entry_++];
    }

    bool HasNext() const {
        return current_entry_ < data_.size();
    }

    void Reset() {
        current_entry_ = 0;
    }
};

}  // namespace model
