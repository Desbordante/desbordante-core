set(NAME fd.fdep)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE fdep.cpp fd_tree_element.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd spdlog::spdlog_header_only Boost::headers
)
