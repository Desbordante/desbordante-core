# --- TaneCommon ---
set(NAME fd.tane.common)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE model/lattice_level.cpp model/lattice_vertex.cpp tane_common.cpp)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::pli Boost::headers
)

# --- Tane ---
set(NAME fd.tane)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE afd_measures.cpp tane.cpp)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::tane::common ${DESBORDANTE_PREFIX}::fd::pli
                    ${DESBORDANTE_PREFIX}::fd::afd_metric Boost::headers
)

# --- PFDTane ---
set(NAME fd.tane.pfd)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE pfdtane.cpp)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::tane::common ${DESBORDANTE_PREFIX}::fd::pli
                    Boost::headers
)
