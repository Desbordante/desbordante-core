set(NAME ac)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE ac_algorithm.cpp ac_exception_finder.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::model::types
            spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::algos better-enums Boost::headers
)
