# ---- Dataset Preparation ----

# Create input data directory (safe for incremental builds)
add_custom_target(
    make-input-data-dir ALL 
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
    COMMENT "Creating data directory"
)

# Extract datasets.zip (triggers on zip file changes)
add_custom_target(
    unZip ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/datasets/datasets.zip
    DEPENDS ${CMAKE_SOURCE_DIR}/datasets/datasets.zip
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
    COMMENT "Unpacking datasets"
    VERBATIM
)
add_dependencies(unZip make-input-data-dir)

# ---- CleanUp ----

# Register dataset artifacts for cleanup
set_property(DIRECTORY 
    APPEND PROPERTY
    ADDITIONAL_CLEAN_FILES 
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/"
)
