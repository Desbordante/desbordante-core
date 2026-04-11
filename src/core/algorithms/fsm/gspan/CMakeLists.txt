set(NAME fsm.gspan)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE gspan.cpp graph_parser.cpp extended_edge.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::algos spdlog::spdlog_header_only magic_enum::magic_enum Boost::headers
)