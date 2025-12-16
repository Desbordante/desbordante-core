set(NAME fd.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE fd_verifier.cpp stats_calculator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::model::table better-enums
                    Boost::headers
)

set(NAME fd.verifier.dynamic)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE dynamic_fd_verifier.cpp dynamic_stats_calculator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only better-enums
                    Boost::headers
)
