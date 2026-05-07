add_subdirectory(condition_miners)
add_subdirectory(cind_verifier)

set(NAME cind)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cind_algorithm.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::cind::miners::cinderella
            ${DESBORDANTE_PREFIX}::cind::miners::pli
            ${DESBORDANTE_PREFIX}::cind::miners::cure
            ${DESBORDANTE_PREFIX}::ind::spider
            better-enums
            spdlog::spdlog_header_only
            Boost::headers
)
