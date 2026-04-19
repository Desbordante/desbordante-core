add_subdirectory(ga_rfd)

set(NAME rfd)
desbordante_add_lib(NAME)

target_sources(${NAME} PRIVATE similarity_metric.cpp)

target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::types Boost::headers better-enums
                    spdlog::spdlog_header_only
)
