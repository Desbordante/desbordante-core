set(NAME ind.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ind_verifier.cpp)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only Boost::headers)
