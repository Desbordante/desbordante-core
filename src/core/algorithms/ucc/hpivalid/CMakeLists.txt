set(NAME ucc.hpivalid)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE hpivalid.cpp hypergraph.cpp result_collector.cpp tree_search.cpp)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only
                    ${DESBORDANTE_PREFIX}::fd::hy::common Boost::headers
)
