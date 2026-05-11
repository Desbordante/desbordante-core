add_subdirectory(ga_rfd)

set(NAME rfd)
desbordante_add_lib(NAME)

target_sources(${NAME} PRIVATE similarity_metric.cpp)

target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::model::types ${DESBORDANTE_PREFIX}::util Boost::headers
            magic_enum::magic_enum spdlog::spdlog_header_only
)
