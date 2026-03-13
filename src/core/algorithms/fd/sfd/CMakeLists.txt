set(NAME fd.sfd.cords)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE contingency_table.cpp cords.cpp frequency_handler.cpp sample.cpp)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd better-enums Boost::headers)
