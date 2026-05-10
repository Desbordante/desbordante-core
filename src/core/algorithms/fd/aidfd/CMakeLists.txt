set(NAME fd.aid)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE aid.cpp search_tree.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::fd Boost::headers
)
