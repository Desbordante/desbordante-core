set(NAME md.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE validation/validation.cpp md_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::md::hy ${DESBORDANTE_PREFIX}::md ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only magic_enum::magic_enum Boost::headers
)
