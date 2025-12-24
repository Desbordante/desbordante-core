set(NAME fd.hy.model)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE model/fd_tree.cpp model/fd_tree_vertex.cpp)
target_link_libraries(${NAME} PRIVATE Boost::headers)

set(NAME fd.hy)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE hyfd.cpp inductor.cpp validator.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::fd::hy::common ${DESBORDANTE_PREFIX}::fd::hy::model
            ${DESBORDANTE_PREFIX}::fd::pli spdlog::spdlog_header_only magic_enum::magic_enum Boost::headers
)
