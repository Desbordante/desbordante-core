set(NAME fd.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fd_verifier.cpp stats_calculator.cpp)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::model::table magic_enum::magic_enum
                    Boost::headers
)

set(NAME fd.verifier.dynamic)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE dynamic_fd_verifier.cpp dynamic_stats_calculator.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only magic_enum::magic_enum
                    Boost::headers
)
