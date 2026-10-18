add_subdirectory(cdd_verifier)

set(NAME cdd)
desbordante_add_lib(NAME INTERFACE)
target_sources(${NAME} INTERFACE FILE_SET HEADERS FILES cdd.h "${PROJECT_SOURCE_DIR}/src")
