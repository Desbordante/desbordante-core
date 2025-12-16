set(NAME cfd.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE cfd_stats_calculator.cpp cfd_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::util ${DESBORDANTE_PREFIX}::cfd::model
            ${DESBORDANTE_PREFIX}::algos spdlog::spdlog_header_only better-enums Boost::headers
)
