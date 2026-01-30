set(NAME parser.csv)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE csv_parser.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE Boost::headers)
