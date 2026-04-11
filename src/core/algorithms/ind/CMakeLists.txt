add_subdirectory(faida)
add_subdirectory(ind_verifier)
add_subdirectory(mind)
add_subdirectory(spider)

set(NAME ind)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE ind.cpp ind_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::algos magic_enum::magic_enum
                    Boost::headers
)
