# ---- Logging Configuration Deployment ----

# Copies the logging configuration file to the output directory
# - ALL: Ensures execution on every build (even incremental)
add_custom_target(
    copy-config ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cfg/logging.conf
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    COMMENT "Deploying logging configuration"
)
