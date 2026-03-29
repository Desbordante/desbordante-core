set(NAME pac.verifier.fd_pac_verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE fd_pac_highlight.cpp fd_pac_verifier.h
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
           ${DESBORDANTE_PREFIX}::model::table
           ${DESBORDANTE_PREFIX}::model::types
           ${DESBORDANTE_PREFIX}::pac
           ${DESBORDANTE_PREFIX}::pac::model
           ${DESBORDANTE_PREFIX}::pac::verifier
           ${DESBORDANTE_PREFIX}::util
    PRIVATE better-enums spdlog::spdlog_header_only
)
