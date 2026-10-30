set(NAME cdd.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cdd_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::dd::verifier ${DESBORDANTE_PREFIX}::cdd
    PRIVATE ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only
            magic_enum::magic_enum
)
