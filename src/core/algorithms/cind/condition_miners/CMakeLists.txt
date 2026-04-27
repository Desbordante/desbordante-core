set(NAME cind.miners)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cinderella.cpp cind_miner.cpp pli_cind.cpp position_lists_set.cpp)
target_link_libraries(${NAME} PRIVATE better-enums spdlog::spdlog_header_only Boost::headers)
