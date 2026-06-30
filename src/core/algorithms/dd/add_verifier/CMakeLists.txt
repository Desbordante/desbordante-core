set(NAME dd.add_verifier)
desbordante_add_lib(NAME)

target_sources(${NAME} PRIVATE add_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::dd ${DESBORDANTE_PREFIX}::dd::verifier
    PRIVATE ${DESBORDANTE_PREFIX}::config spdlog::spdlog_header_only magic_enum::magic_enum
            Boost::headers
)
