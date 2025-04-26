# ---- Examples Configuration ----

# Set the name/path of the examples directory
# This should match the actual folder name in the source tree
set(EXAMPLES_DIR "examples")

# Create a custom target that establishes a symlink to the examples directory
# - Uses CMake's built-in create_symlink command for cross-platform compatibility
# - ALL ensures this runs with every build (including incremental builds)
# - The symlink points from build output dir back to source dir for live updates
add_custom_target(
    symlink-python-examples ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/${EXAMPLES_DIR}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXAMPLES_DIR}
    COMMENT
        "Creating examples symlink: $<TARGET_FILE_DIR:symlink-python-examples>/${EXAMPLES_DIR} -> ${CMAKE_SOURCE_DIR}/${EXAMPLES_DIR}"
)
    
# ---- CleanUp Configuration ----

# Special target to safely remove just the symlink
add_custom_target(
    clean-symlink-examples
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXAMPLES_DIR}
    COMMENT "Removing examples symlink (safe cleanup - original files preserved)"
    VERBATIM
)

# Connect custom cleanup to the standard CMake clean process
add_dependencies(clean clean-symlink-examples)
