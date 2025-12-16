set(NAME metric.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE highlight_calculator.cpp metric_verifier.cpp points_calculator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only better-enums Boost::headers
)
