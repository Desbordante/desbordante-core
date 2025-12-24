set(NAME dd.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE dd_verifier.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::dd
            ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            spdlog::spdlog_header_only
            ${DESBORDANTE_PREFIX}::algos
            magic_enum::magic_enum
            Boost::headers
)
