set(NAME fem.maxfem.model)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE model/bound_list.cpp model/composite_episode.cpp model/location_list.cpp
                               model/max_episodes_collection.cpp model/parallel_episode.cpp)

set(NAME fem.maxfem)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE maxfem.cpp composite_episode_miner.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::fem::maxfem::model
            ${DESBORDANTE_PREFIX}::fem
            ${DESBORDANTE_PREFIX}::model::sequence
            spdlog::spdlog_header_only Boost::thread
)
