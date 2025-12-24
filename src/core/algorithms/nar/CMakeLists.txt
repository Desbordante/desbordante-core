add_subdirectory(des)

set(NAME nar)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE nar.cpp nar_algorithm.cpp value_range.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table magic_enum::magic_enum Boost::headers
)
