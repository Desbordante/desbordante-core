set(NAME parser.spmf)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE spmf_parser.cpp)
target_link_libraries(${NAME} PRIVATE Boost::headers)
