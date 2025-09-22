# ---- Core Library Configuration ----

# Define the library name
set(BINARY ${CMAKE_PROJECT_NAME})

# Collect all source files recursively (headers and implementations)
file(GLOB_RECURSE lib_sources "*/*.h*" "*/*.cpp*")

# Configure threading support with pthread preference
# Required for Boost.Thread and other thread-dependent components
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Build core project as a static library
add_library(${BINARY} STATIC ${lib_sources})

target_compile_definitions(${BINARY} PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
        $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
)

# Link essential dependencies
# PRIVATE: Boost and threading internals (implementation details)
# PUBLIC: spdlog affects public interface (logging in headers)
target_link_libraries(
    ${BINARY}
    PRIVATE ${Boost_LIBRARIES} # Boost components specified in find_package
            Threads::Threads # System threading library
    PUBLIC spdlog::spdlog_header_only # Logging infrastructure exposed in headers
)

# ---- Vertical Hashing Optimization ----

# Performance/functionality trade-off:
# - Disabled by default for maximum performance
# - Enabled for wide datasets (>32 columns) with potential performance penalty
option(SAFE_VERTICAL_HASHING
       "Enable safe vertical hashing mode. Allows processing wide datasets (>32 columns) \
    at the cost of reduced performance. Recommended for exploratory data analysis." OFF
)

# Propagate the setting to all consumers of the library
if(SAFE_VERTICAL_HASHING)
    target_compile_definitions(
        ${BINARY} PUBLIC SAFE_VERTICAL_HASHING # Affects compilation of dependent code
    )
endif()
