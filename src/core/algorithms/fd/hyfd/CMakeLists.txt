set(NAME fd.hy.model)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/fd_tree.cpp model/fd_tree_vertex.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE Boost::headers)

set(NAME fd.hy)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE hyfd.cpp inductor.cpp validator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::fd::hy::common ${DESBORDANTE_PREFIX}::fd::hy::model
            ${DESBORDANTE_PREFIX}::fd::pli spdlog::spdlog_header_only better-enums Boost::headers
)
