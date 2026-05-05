set(NAME cind.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cind_verifier.cpp)
target_link_libraries(${NAME} PRIVATE better-enums spdlog::spdlog_header_only Boost::headers)
