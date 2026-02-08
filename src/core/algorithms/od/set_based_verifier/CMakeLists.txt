set(NAME od.set_based_aod_verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only better-enums Boost::headers)
