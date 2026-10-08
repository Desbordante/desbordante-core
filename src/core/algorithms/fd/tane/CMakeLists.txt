# --- Tane ---
set(NAME afd.tane)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE tane.cpp tane_initial.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::afd ${DESBORDANTE_PREFIX}::afd::pli
    PRIVATE ${DESBORDANTE_PREFIX}::afd::metric_calc Boost::headers
)
