set(NAME fd.pyro)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE pyro.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::pyro::common
                    ${DESBORDANTE_PREFIX}::fd::pli better-enums Boost::headers
)
