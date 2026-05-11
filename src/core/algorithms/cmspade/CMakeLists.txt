add_subdirectory(model)
add_subdirectory(parser)

set(NAME cmspade)
desbordante_add_lib(NAME)

target_sources(
    ${NAME}
    PRIVATE cmspade.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(
    ${NAME}
    PUBLIC
        ${DESBORDANTE_PREFIX}::cmspade::model
        ${DESBORDANTE_PREFIX}::cmspade::parser
        Boost::headers
    PRIVATE
        ${DESBORDANTE_PREFIX}::algos better-enums
)