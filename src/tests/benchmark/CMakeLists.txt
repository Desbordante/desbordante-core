# Create benchmark executable
set(NAME ${DESBORDANTE_PREFIX}.benchmark)
add_executable(${NAME})
target_sources(${NAME} PRIVATE main.cpp benchmark_cli.cpp benchmark_comparer.cpp)
target_include_directories(${NAME} PRIVATE ${DESBORDANTE_COMMON_INCLUDE_DIRS})
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::testlib::common
            ${DESBORDANTE_PREFIX}::dc::fastadc
            ${DESBORDANTE_PREFIX}::dd::split
            ${DESBORDANTE_PREFIX}::ind
            ${DESBORDANTE_PREFIX}::ind::mind
            ${DESBORDANTE_PREFIX}::fd::hy
            ${DESBORDANTE_PREFIX}::fd::pyro
            ${DESBORDANTE_PREFIX}::fd::tane
            ${DESBORDANTE_PREFIX}::fd::aid
            ${DESBORDANTE_PREFIX}::md::hy
            ${DESBORDANTE_PREFIX}::md::hy::preprocessing
            ${DESBORDANTE_PREFIX}::nar::des
            ${DESBORDANTE_PREFIX}::rfd
            ${DESBORDANTE_PREFIX}::rfd::ga_rfd
            Boost::program_options
            magic_enum::magic_enum
            spdlog::spdlog_header_only
)
add_dependencies(
    ${NAME} Desbordante.zip.datasets Desbordante.zip.highway-neighbors
    Desbordante.zip.performance-tests
)

add_executable(Desbordante.ga_rfd_tuner ga_rfd_tuner.cpp)
target_include_directories(Desbordante.ga_rfd_tuner PRIVATE ${DESBORDANTE_COMMON_INCLUDE_DIRS})
target_link_libraries(
    Desbordante.ga_rfd_tuner
    PRIVATE ${DESBORDANTE_PREFIX}::testlib::common
            ${DESBORDANTE_PREFIX}::rfd::ga_rfd
            ${DESBORDANTE_PREFIX}::rfd
            ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            Boost::headers
            spdlog::spdlog_header_only
            better-enums
)
