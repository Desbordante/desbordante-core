set(NAME ucc.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ucc_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::model::table magic_enum::magic_enum Boost::headers
)
