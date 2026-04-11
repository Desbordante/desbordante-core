set(NAME cfd.cfun)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE cfun.cpp
            quadruple.cpp
            attribute_utils.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::util ${DESBORDANTE_PREFIX}::cfd::model
            ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::fd spdlog::spdlog_header_only better-enums Boost::headers
)
