set(NAME fd.pyro)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE pyro.cpp)
target_link_libraries(
    ${NAME} PUBLIC ${DESBORDANTE_PREFIX}::afd ${DESBORDANTE_PREFIX}::afd::pli
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::pyro::common Boost::headers
)
