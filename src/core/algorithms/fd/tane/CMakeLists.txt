# --- TaneCommon ---
set(NAME fd.tane.common)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/lattice_level.cpp model/lattice_vertex.cpp tane_common.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::fd::pli Boost::headers
)

# --- Tane ---
set(NAME fd.tane)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE afd_measures.cpp tane.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::tane::common ${DESBORDANTE_PREFIX}::fd::pli
                    better-enums Boost::headers
)

# --- PFDTane ---
set(NAME fd.tane.pfd)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE pfdtane.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::tane::common ${DESBORDANTE_PREFIX}::fd::pli
                    better-enums Boost::headers
)
