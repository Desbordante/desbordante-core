add_subdirectory(split)
add_subdirectory(dd_verifier)
add_subdirectory(add_verifier)

set(NAME dd)
desbordante_add_lib(NAME INTERFACE)
target_sources(${NAME} INTERFACE FILE_SET HEADERS FILES dd.h)
