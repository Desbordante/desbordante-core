add_subdirectory(maxfem)

set(NAME fem)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE fem_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::sequence ${DESBORDANTE_PREFIX}::algos Boost::headers
)
