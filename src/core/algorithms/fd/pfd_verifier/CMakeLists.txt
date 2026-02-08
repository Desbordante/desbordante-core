set(NAME fd.pfd_verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE pfd_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table better-enums Boost::headers
)
