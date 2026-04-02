set(NAME fd.fun)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fun.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::pli spdlog::spdlog_header_only Boost::headers
)
