add_subdirectory(ga_rfd)

set(NAME rfd)
desbordante_add_lib(NAME)

target_sources(${NAME} PRIVATE similarity_metric.cpp)

target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::util
    PRIVATE Boost::headers
)
