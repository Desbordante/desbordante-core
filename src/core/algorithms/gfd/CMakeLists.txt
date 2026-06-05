add_subdirectory(gfd_miner)
add_subdirectory(gfd_validator)

set(NAME gfd)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE comparator.cpp gfd.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::util Boost::headers
)
