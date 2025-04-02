add_custom_target(make-input-data-dir ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
)

add_custom_target(unZip ALL 
	COMMAND ${CMAKE_COMMAND} -E tar xzf
		${CMAKE_SOURCE_DIR}/datasets/datasets.zip
	DEPENDS
		${CMAKE_SOURCE_DIR}/datasets/datasets.zip
	WORKING_DIRECTORY
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
	COMMENT "Unpacking datasets.zip"
	VERBATIM
)

add_dependencies(unZip make-input-data-dir)
