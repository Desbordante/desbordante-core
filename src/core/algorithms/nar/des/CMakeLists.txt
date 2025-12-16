set(NAME nar.des)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE des.cpp differential_functions.cpp encoded_nar.cpp encoded_value_range.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::nar better-enums Boost::headers)
