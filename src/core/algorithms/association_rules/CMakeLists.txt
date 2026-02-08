set(NAME ar)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE apriori.cpp ar_algorithm.cpp candidate_hash_tree.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::transaction ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only better-enums Boost::headers
)
