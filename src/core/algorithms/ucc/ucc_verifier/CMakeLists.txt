set(NAME ucc.verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE ucc_verifier.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE better-enums Boost::headers)
