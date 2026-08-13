set(NAME rfd.ga_rfd)
desbordante_add_lib(NAME)

target_sources(${NAME} PRIVATE ga_rfd.cpp)

target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::rfd
           ${DESBORDANTE_PREFIX}::util
    PRIVATE Boost::headers spdlog::spdlog_header_only
)
