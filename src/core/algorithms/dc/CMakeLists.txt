add_subdirectory(FastADC)
add_subdirectory(verifier)

set(NAME dc.model)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE model/component.cpp model/dc.cpp)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::algos spdlog::spdlog_header_only Boost::headers
)

set(NAME dc.parser)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE parser/dc_parser.cpp)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only magic_enum::magic_enum Boost::headers)
