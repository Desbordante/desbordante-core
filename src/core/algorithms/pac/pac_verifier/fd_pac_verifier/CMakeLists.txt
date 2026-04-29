set(NAME pac.verifier.fd_pac_verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE fd_pac_highlight.cpp fd_pac_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::model::types
           ${DESBORDANTE_PREFIX}::pac ${DESBORDANTE_PREFIX}::pac::model
           ${DESBORDANTE_PREFIX}::pac::verifier
    PRIVATE ${DESBORDANTE_PREFIX}::util better-enums ${DESBORDANTE_PREFIX}::model::table
            spdlog::spdlog_header_only
)
