set(NAME model.types)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE create_type.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::util better-enums Boost::headers)
