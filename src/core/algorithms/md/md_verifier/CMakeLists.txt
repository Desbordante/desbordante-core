set(NAME md.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE validation/validation.cpp md_verifier.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::md::hy ${DESBORDANTE_PREFIX}::md
                    spdlog::spdlog_header_only magic_enum::magic_enum Boost::headers
)
