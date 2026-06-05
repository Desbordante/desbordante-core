set(NAME cind.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cind_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::util magic_enum::magic_enum spdlog::spdlog_header_only
            Boost::headers
)
