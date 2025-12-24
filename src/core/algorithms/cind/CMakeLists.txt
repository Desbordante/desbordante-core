add_subdirectory(condition_miners)
add_subdirectory(cind_verifier)

set(NAME cind)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cind_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::cind::miners ${DESBORDANTE_PREFIX}::ind::spider
                    magic_enum::magic_enum spdlog::spdlog_header_only Boost::headers
)
