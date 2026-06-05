add_subdirectory(hymd)
add_subdirectory(md_verifier)

set(NAME md)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE md.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::util Boost::headers
)
