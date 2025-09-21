set(NAME pac.model.default_domains)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE ball.cpp metric_based_domain.cpp parallelepiped.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::pac::model ${DESBORDANTE_PREFIX}::model::types better-enums
           ${DESBORDANTE_PREFIX}::config
    PRIVATE Boost::headers
)
