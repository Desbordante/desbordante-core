set(NAME fem.tke.model)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} PRIVATE model/bound_list.cpp model/composite_episode.cpp model/location_list.cpp
                    model/parallel_episode.cpp
)

set(NAME fem.tke)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE tke.cpp composite_topk_miner.cpp parallel_topk_miner.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fem::tke::model ${DESBORDANTE_PREFIX}::fem
                    ${DESBORDANTE_PREFIX}::model::sequence spdlog::spdlog_header_only Boost::thread
)
