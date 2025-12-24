# --- AFD metrics ---
set(NAME fd.afd_metric)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE afd_metric_calculator.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC magic_enum::magic_enum
    PRIVATE Boost::headers
)
