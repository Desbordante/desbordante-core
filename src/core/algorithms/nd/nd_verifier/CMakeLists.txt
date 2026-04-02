set(NAME nd.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE nd_verifier.cpp util/highlight.cpp util/stats_calculator.cpp
                    util/value_combination.cpp
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only better-enums Boost::headers
)
