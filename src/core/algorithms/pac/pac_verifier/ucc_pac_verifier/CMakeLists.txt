set(NAME pac.verifier.ucc_pac_verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ucc_pac_highlight.cpp ucc_pac_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::model::types
           ${DESBORDANTE_PREFIX}::pac ${DESBORDANTE_PREFIX}::pac::model
           ${DESBORDANTE_PREFIX}::pac::verifier
    PRIVATE ${DESBORDANTE_PREFIX}::util better-enums ${DESBORDANTE_PREFIX}::model::table
            spdlog::spdlog_header_only
)
