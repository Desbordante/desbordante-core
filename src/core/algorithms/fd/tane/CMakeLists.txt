# --- Tane ---
set(NAME afd.tane)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE tane.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::afd ${DESBORDANTE_PREFIX}::afd::pli
    PRIVATE spdlog::spdlog_header_only Boost::headers ${DESBORDANTE_PREFIX}::afd::metric_calc
)
