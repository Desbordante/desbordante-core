set(NAME fd.depminer)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE depminer.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::pli spdlog::spdlog_header_only Boost::headers
)
