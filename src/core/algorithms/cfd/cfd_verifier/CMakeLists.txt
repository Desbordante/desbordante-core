set(NAME cfd.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cfd_stats_calculator.cpp cfd_verifier.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::util
            ${DESBORDANTE_PREFIX}::cfd::model
            ${DESBORDANTE_PREFIX}::algos
            ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only
            magic_enum::magic_enum
            Boost::headers
)
