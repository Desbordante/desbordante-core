# ---- Logging Configuration Deployment ----

# Copies the logging configuration file to the output directory
# - ALL: Ensures execution on every build (even incremental)
add_custom_target(
    copy-config ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cfg/logging.conf
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    COMMENT "Deploying logging configuration"
)

# ---- CleanUp Configuration ----

# Register logging.conf for removal during cleanup phase
# - File will be deleted when running: 
#   `cmake --build <build_dir> --target clean`
set_property(DIRECTORY APPEND PROPERTY
    ADDITIONAL_CLEAN_FILES
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/logging.conf
)
