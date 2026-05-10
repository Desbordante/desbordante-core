set(NAME fd.euler)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cluster.cpp eulerfd.cpp mlfq.cpp search_tree.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::fd Boost::headers
)
