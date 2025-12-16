# Create benchmark executable
set(NAME ${DESBORDANTE_PREFIX}.benchmark)
add_executable(${NAME})
target_sources(
    ${NAME}
    PRIVATE main.cpp benchmark_cli.cpp benchmark_comparer.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
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
            Boost::program_options
            better-enums
            frozen
            spdlog::spdlog_header_only
)
add_dependencies(
    ${NAME} Desbordante.zip.datasets Desbordante.zip.highway-neighbors
    Desbordante.zip.performance-tests
)
