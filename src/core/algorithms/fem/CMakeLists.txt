add_subdirectory(afem)
add_subdirectory(maxfem)
add_subdirectory(tke)

set(NAME fem)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE fem_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::sequence ${DESBORDANTE_PREFIX}::algos
                    Boost::headers
)
