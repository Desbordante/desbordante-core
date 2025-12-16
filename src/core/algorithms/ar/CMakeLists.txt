add_subdirectory(ar_verifier)

set(NAME ar)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE apriori/apriori.cpp ar_algorithm.cpp apriori/candidate_hash_tree.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::transaction ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only better-enums Boost::headers
)

