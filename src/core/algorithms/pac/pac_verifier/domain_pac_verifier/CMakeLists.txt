set(NAME pac.verifier.domain_pac_verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE domain_pac_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::pac::model ${DESBORDANTE_PREFIX}::model::table
           ${DESBORDANTE_PREFIX}::model::types ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::pac::model::default_domains ${DESBORDANTE_PREFIX}::pac::verifier
            ${DESBORDANTE_PREFIX}::util Boost::headers better-enums spdlog::spdlog_header_only
)
