# ---- Core Library Configuration ----

add_subdirectory(model/table)
add_subdirectory(model/transaction)
add_subdirectory(model/types)
add_subdirectory(algorithms)
add_subdirectory(config)
add_subdirectory(parser/csv_parser)
add_subdirectory(parser/graph_parser)
add_subdirectory(util)

# Configure threading support with pthread preference
# Required for Boost.Thread and other thread-dependent components
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ---- Vertical Hashing Optimization ----

# Performance/functionality trade-off:
# - Disabled by default for maximum performance
# - Enabled for wide datasets (>32 columns) with potential performance penalty
option(SAFE_VERTICAL_HASHING
       "Enable safe vertical hashing mode. Allows processing wide datasets (>32 columns) \
    at the cost of reduced performance. Recommended for exploratory data analysis." OFF
)
