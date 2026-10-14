set(NAME gfd.miner)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE gfd_miner.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::config
            ${DESBORDANTE_PREFIX}::parser::graph
            ${DESBORDANTE_PREFIX}::gfd
            ${DESBORDANTE_PREFIX}::algos
            ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only
            Boost::headers
)
