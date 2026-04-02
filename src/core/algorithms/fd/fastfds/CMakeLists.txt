set(NAME fd.fastfds)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fastfds.cpp)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::pli Boost::headers
)
