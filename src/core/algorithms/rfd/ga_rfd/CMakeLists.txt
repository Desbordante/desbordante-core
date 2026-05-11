set(NAME rfd.ga_rfd)
desbordante_add_lib(NAME)

target_sources(${NAME} PRIVATE ga_rfd.cpp)

target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::rfd ${DESBORDANTE_PREFIX}::util Boost::headers
            magic_enum::magic_enum spdlog::spdlog_header_only
)
