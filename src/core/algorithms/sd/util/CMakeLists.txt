set(NAME sd.util)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE data_structures.cpp edit_distance_calculator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only Boost::headers magic_enum::magic_enum
)
