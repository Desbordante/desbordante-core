set(NAME ucc.pyro)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE pyroucc.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::ucc ${DESBORDANTE_PREFIX}::fd::pyro::common
                    ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only better-enums Boost::headers
)
