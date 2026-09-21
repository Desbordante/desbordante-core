set(NAME cfd.ctane)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ctane.cpp c_lattice_level.cpp c_lattice_vertex.cpp tuple_pattern.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::cfd
    PRIVATE ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only
            Boost::headers
)
