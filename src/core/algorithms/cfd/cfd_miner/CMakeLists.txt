set(NAME cfd.cfdminer)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cfd_miner.cpp gc_growth.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::cfd
    PRIVATE ${DESBORDANTE_PREFIX}::algos Boost::headers
)
