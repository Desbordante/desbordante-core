set(NAME fd.aid)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE aid.cpp search_tree.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd Boost::headers)
