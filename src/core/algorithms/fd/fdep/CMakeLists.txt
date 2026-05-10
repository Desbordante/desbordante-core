set(NAME fd.fdep)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fdep.cpp fd_tree_element.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::fd spdlog::spdlog_header_only Boost::headers
)
