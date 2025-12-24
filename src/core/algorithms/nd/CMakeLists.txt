add_subdirectory(nd_verifier)

set(NAME nd)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE nd.cpp util/get_vertical_names.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE Boost::headers)
