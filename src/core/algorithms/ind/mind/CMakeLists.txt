set(NAME ind.mind)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE mind.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::create_algo spdlog::spdlog_header_only better-enums
                    Boost::headers
)
