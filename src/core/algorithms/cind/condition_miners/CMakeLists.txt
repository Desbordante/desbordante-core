set(NAME cind.miners)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cinderella.cpp cind_miner.cpp pli_cind.cpp position_lists_set.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::util magic_enum::magic_enum spdlog::spdlog_header_only
            Boost::headers
)
