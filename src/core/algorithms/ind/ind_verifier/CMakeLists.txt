set(NAME ind.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ind_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only Boost::headers
)
