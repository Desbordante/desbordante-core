add_subdirectory(default_domains)

set(NAME pac.model)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE tuple.cpp tuple_type.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::model::types
    PRIVATE Boost::headers better-enums
)
