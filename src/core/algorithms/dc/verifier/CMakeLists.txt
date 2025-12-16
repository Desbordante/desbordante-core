set(NAME dc.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE dc_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            ${DESBORDANTE_PREFIX}::dc::model
            ${DESBORDANTE_PREFIX}::dc::parser
            spdlog::spdlog_header_only
            frozen
            better-enums
            Boost::headers
)
