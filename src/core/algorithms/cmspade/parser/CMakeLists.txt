set(NAME cmspade.parser)
desbordante_add_lib(NAME OBJECT)

target_sources(
    ${NAME}
    PRIVATE
        cmspade_parser.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::cmspade::model
)