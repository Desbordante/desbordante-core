set(NAME ar)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE apriori.cpp ar_algorithm.cpp candidate_hash_tree.cpp)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::transaction ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only Boost::headers
)
