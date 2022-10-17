#pragma once

#include <list>
#include "model/partial_fd.h"
#include "model/partial_key.h"

class DependencyStorage {
public:
    std::list<PartialFD> discovered_fds_;
    std::list<PartialKey> discovered_uccs_;
};