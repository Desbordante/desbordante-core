add_subdirectory(gdd_validator)

set(NAME gdd)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE gdd.cpp)
target_link_libraries(${NAME} PRIVATE Boost::headers)
