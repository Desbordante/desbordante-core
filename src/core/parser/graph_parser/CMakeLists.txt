set(NAME parser.graph)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE graph_parser.cpp)
target_link_libraries(${NAME} PRIVATE Boost::graph)
