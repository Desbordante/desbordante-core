set(NAME ind.spider)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE spider.cpp attribute.cpp)
target_link_libraries(${NAME} PRIVATE magic_enum::magic_enum Boost::headers)
