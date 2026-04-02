set(NAME gfd.miner)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE gfd_miner.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::parser::graph ${DESBORDANTE_PREFIX}::gfd
            ${DESBORDANTE_PREFIX}::algos spdlog::spdlog_header_only better-enums Boost::headers
)
