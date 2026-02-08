add_subdirectory(hpivalid)
add_subdirectory(hyucc)
add_subdirectory(pyroucc)
add_subdirectory(ucc_verifier)

set(NAME ucc)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE ucc_algorithm.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE Boost::headers)
