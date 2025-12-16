set(NAME ar.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE ar_verifier.cpp ar_stats_calculator.cpp model/rule_coverage.cpp
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::ar ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only better-enums Boost::headers
)
