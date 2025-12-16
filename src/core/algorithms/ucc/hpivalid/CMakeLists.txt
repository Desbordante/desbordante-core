set(NAME ucc.hpivalid)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE hpivalid.cpp hypergraph.cpp result_collector.cpp tree_search.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::hy::common better-enums
                    Boost::headers
)
