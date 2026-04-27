set(NAME parser.sequence)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE file_sequence_parser.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::sequence Boost::headers
)
