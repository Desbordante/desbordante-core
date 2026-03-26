add_subdirectory(domain_pac_verifier)
add_subdirectory(fd_pac_verifier)

set(NAME pac.verifier)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE pac_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config
           ${DESBORDANTE_PREFIX}::model::table
    PRIVATE Boost::headers better-enums spdlog::spdlog_header_only
)
