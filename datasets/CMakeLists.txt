add_custom_target(make-input-data-dir)
add_custom_command(
	TARGET make-input-data-dir
	COMMAND ${CMAKE_COMMAND} -E make_directory
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
)

add_custom_target(unZip ALL)
add_custom_command(
	TARGET unZip
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

if (COMPILE_PERFORMANCE_TESTS)
	# Please, don't forget to add here names of datasets you want to produce
	set(cropped_dataset_names
		adult5attr.csv CIPublicHighway20attr60k.csv iowa450k.csv mushroom+3attr1k.csv
		mushroom+4attr1k.csv)

	add_custom_target(cropDatasets ALL DEPENDS ${cropped_dataset_names})
	add_custom_command(
		OUTPUT ${cropped_dataset_names}
		COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/datasets/crop_datasets.sh
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
	)

	add_dependencies(cropDatasets unZip)
endif()
