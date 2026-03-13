set(NAME sd.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE sd_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::sd::util ${DESBORDANTE_PREFIX}::model::table better-enums Boost::headers
)

set(NAME sd.util)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE util/data_structures.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE spdlog::spdlog_header_only better-enums Boost::headers
)
