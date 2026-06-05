add_subdirectory(hpivalid)
add_subdirectory(hyucc)
add_subdirectory(pyroucc)
add_subdirectory(ucc_verifier)

set(NAME ucc)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE ucc_algorithm.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE Boost::headers
)
