add_subdirectory(split)
add_subdirectory(dd_verifier)

set(NAME dd)
desbordante_add_lib(NAME INTERFACE)
target_sources(${NAME} INTERFACE FILE_SET HEADERS FILES dd.h BASE_DIRS "${PROJECT_SOURCE_DIR}/src")
