set(NAME rfd.ga_rfd)
desbordante_add_lib(NAME OBJECT)

target_sources(${NAME} PRIVATE
    ga_rfd.cpp
)

target_link_libraries(${NAME} PRIVATE
    ${DESBORDANTE_PREFIX}::rfd
    Boost::headers
    better-enums
    spdlog::spdlog_header_only
)