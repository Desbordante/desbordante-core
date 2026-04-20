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

find_package(ICU REQUIRED COMPONENTS uc i18n)

# Build core project as a static library
add_library(${BINARY} STATIC ${lib_sources})

# Set log level based on user input or build type
if(DESBORDANTE_LOG_LEVEL)
    set(SPDLOG_LEVEL "SPDLOG_LEVEL_${DESBORDANTE_LOG_LEVEL}")
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SPDLOG_LEVEL "SPDLOG_LEVEL_DEBUG")
    else()
        set(SPDLOG_LEVEL "SPDLOG_LEVEL_INFO")
    endif()
endif()

target_compile_definitions(${BINARY} PUBLIC
        SPDLOG_ACTIVE_LEVEL=${SPDLOG_LEVEL}
)

# Link essential dependencies
# PRIVATE: Boost and threading internals (implementation details)
# PUBLIC: spdlog affects public interface (logging in headers)
target_link_libraries(
    ${BINARY}
    PRIVATE ${Boost_LIBRARIES} # Boost components specified in find_package
            Threads::Threads # System threading library
            ICU::uc
            ICU::i18n
    PUBLIC spdlog::spdlog_header_only # Logging infrastructure exposed in headers
)

# Propagate the setting to all consumers of the library
if(DESBORDANTE_SAFE_VERTICAL_HASHING)
    target_compile_definitions(
        ${BINARY} PUBLIC DESBORDANTE_SAFE_VERTICAL_HASHING # Affects compilation of dependent code
    )
endif()

