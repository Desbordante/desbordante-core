set(NAME dd.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE dd_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::algos
            ${DESBORDANTE_PREFIX}::dd
            ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only
            magic_enum::magic_enum
            Boost::headers
)
