set(NAME model.types)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE create_type.cpp)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::util better-enums Boost::headers)
