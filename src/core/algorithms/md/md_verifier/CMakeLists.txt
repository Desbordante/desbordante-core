set(NAME md.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE validation/validation.cpp md_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::md::hy ${DESBORDANTE_PREFIX}::md
                    spdlog::spdlog_header_only better-enums Boost::headers
)
