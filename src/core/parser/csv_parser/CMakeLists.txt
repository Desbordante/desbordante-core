set(NAME parser.csv)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE csv_parser.cpp)
target_link_libraries(${NAME} PRIVATE Boost::headers)
