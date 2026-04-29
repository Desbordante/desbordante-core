add_subdirectory(model)
add_subdirectory(pac_verifier)

set(NAME pac)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE domain_pac.h fd_pac.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::model::table Boost::headers
    PRIVATE better-enums
)
