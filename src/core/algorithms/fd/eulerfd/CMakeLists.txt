set(NAME fd.euler)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE cluster.cpp eulerfd.cpp mlfq.cpp search_tree.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd Boost::headers)
