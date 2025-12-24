set(NAME ucc.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ucc_verifier.cpp)
target_link_libraries(${NAME} PRIVATE magic_enum::magic_enum Boost::headers)
