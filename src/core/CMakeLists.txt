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
