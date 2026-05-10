# --- AFD metrics ---
set(NAME fd.afd_metric)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE afd_metric_calculator.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config magic_enum::magic_enum
    PRIVATE ${DESBORDANTE_PREFIX}::util Boost::headers
)
