set(NAME gdd.graph_matcher)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE bfs_qvo_strategy.cpp cost_based_qvo_strategy.cpp backtracking_graph_matcher.cpp
                    wcoj_graph_matcher.cpp
)
target_link_libraries(${NAME} PRIVATE Boost::headers)
