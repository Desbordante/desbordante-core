set(NAME ucc.hy)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE hyucc.cpp inductor.cpp validator.cpp model/ucc_tree.cpp model/ucc_tree_vertex.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::config
            ${DESBORDANTE_PREFIX}::fd::hy::common ${DESBORDANTE_PREFIX}::ucc
            ${DESBORDANTE_PREFIX}::algos Boost::headers
)
