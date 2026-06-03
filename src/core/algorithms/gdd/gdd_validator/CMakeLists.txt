set(NAME gdd.validator)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE gdd_validator.cpp naive_gdd_validator.cpp wcoj_gdd_validator.cpp bfs_qvo_strategy.cpp cost_based_qvo_strategy.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::parser::graph spdlog::spdlog_header_only
                    ${DESBORDANTE_PREFIX}::gdd ${DESBORDANTE_PREFIX}::algos Boost::headers
)
