set(NAME nar.des)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE des.cpp differential_functions.cpp encoded_nar.cpp encoded_value_range.cpp
)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::nar Boost::headers)
