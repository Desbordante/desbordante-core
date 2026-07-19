set(NAME fem.afem)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE afem.cpp composite_episode_miner.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fem::maxfem::model ${DESBORDANTE_PREFIX}::fem
                    ${DESBORDANTE_PREFIX}::model::sequence spdlog::spdlog_header_only Boost::thread
)
