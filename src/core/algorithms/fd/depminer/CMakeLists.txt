set(NAME fd.depminer)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE depminer.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::pli spdlog::spdlog_header_only Boost::headers
)
