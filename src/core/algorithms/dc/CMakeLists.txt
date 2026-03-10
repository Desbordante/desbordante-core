add_subdirectory(fast_adc)
add_subdirectory(dc_verifier)
add_subdirectory(adc_verifier)

set(NAME dc.model)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/component.cpp model/dc.cpp measures/measure.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::algos spdlog::spdlog_header_only frozen better-enums
                    Boost::headers
)

set(NAME dc.parser)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE parser/dc_parser.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only frozen better-enums Boost::headers)
