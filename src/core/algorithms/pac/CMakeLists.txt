add_subdirectory(model)
add_subdirectory(pac_verifier)

set(NAME pac)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE pac.h domain_pac.h fd_pac.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::model::table
           ${DESBORDANTE_PREFIX}::pac::model Boost::headers
    PRIVATE better-enums
)
