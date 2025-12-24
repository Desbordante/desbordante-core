set(NAME ar.apriori)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE apriori.cpp candidate_hash_tree.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::ar ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only magic_enum::magic_enum Boost::headers
)
