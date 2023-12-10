add_custom_target(symlink-python-examples ALL)
set(EXAMPLES_DIR "examples")
add_custom_command(TARGET symlink-python-examples
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/${EXAMPLES_DIR}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXAMPLES_DIR}
        COMMENT "mklink ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXAMPLES_DIR} -> ${CMAKE_SOURCE_DIR}/${EXAMPLES_DIR}"
)
