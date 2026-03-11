add_subdirectory(default_domains)

set(NAME pac.model)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE tuple_type.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::model::types
    PRIVATE Boost::headers better-enums
)
