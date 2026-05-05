set(NAME cind.miners.common)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cind_miner.cpp)
target_link_libraries(${NAME} PRIVATE better-enums spdlog::spdlog_header_only Boost::headers)

set(NAME cind.miners.cinderella)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cinderella.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::cind::miners::common better-enums
                    spdlog::spdlog_header_only Boost::headers
)

set(NAME cind.miners.pli)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE pli_cind.cpp position_lists_set.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::cind::miners::common better-enums
                    spdlog::spdlog_header_only Boost::headers
)

set(NAME cind.miners.cure)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cure_cind.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::cind::miners::common better-enums
                    spdlog::spdlog_header_only Boost::headers
)
