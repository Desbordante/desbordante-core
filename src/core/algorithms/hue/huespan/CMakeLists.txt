set(NAME hue.huespan)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE huespan.cpp complex_utility_sequence.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config
                    ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only Boost::headers
)
target_include_directories(${NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")