set(NAME fd.hy.common)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE all_column_combinations.cpp preprocessor.cpp sampler.cpp validator_helpers.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE Boost::thread ${DESBORDANTE_PREFIX}::fd::hy::model spdlog::spdlog_header_only
)
