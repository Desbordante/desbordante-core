add_custom_target(
	copy-config ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/cfg/logging.conf ${CMAKE_SOURCE_DIR}/cfg/pg_config.json
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
)
