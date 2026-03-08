# --- AFD metrics ---
set(NAME fd.afd_metric)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE afd_metric_calculator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC better-enums
    PRIVATE Boost::headers
)
