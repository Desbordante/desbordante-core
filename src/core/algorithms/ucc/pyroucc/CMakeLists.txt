set(NAME ucc.pyro)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE pyroucc.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::ucc ${DESBORDANTE_PREFIX}::fd::pyro::common
            ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only better-enums
            Boost::headers
)
