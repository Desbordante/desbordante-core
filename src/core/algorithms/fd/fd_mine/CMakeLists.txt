set(NAME fd.mine)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fd_mine.cpp)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::pli Boost::headers
)
