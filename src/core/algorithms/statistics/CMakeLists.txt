set(NAME datastats)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE data_stats.cpp statistic.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::model::types
                    ${DESBORDANTE_PREFIX}::algos better-enums Boost::headers
)
