set(NAME fd.sfd.cords)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE contingency_table.cpp cords.cpp frequency_handler.cpp sample.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd better-enums Boost::headers)
