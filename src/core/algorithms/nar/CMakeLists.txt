add_subdirectory(des)

set(NAME nar)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE nar.cpp nar_algorithm.cpp value_range.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table better-enums Boost::headers
)
