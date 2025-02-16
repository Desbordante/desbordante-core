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

if (COMPILE_BENCHMARKS)
	# Please, don't forget to add here names of datasets you want to produce
	set(cropped_dataset_names
		adult9attr.csv CIPublicHighway20attr55k.csv iowa550k.csv iowa650k.csv
		mushroom+3attr1500.csv mushroom+4attr1300.csv neighbors120k.csv)

	add_custom_target(cropDatasets ALL DEPENDS ${cropped_dataset_names})
	add_custom_command(
		OUTPUT ${cropped_dataset_names}
		COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/datasets/crop_datasets.sh
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/
	)

	add_dependencies(cropDatasets unZip)
endif()

# ---- CleanUp ----

# Register dataset artifacts for cleanup
set_property(DIRECTORY 
    APPEND PROPERTY
    ADDITIONAL_CLEAN_FILES 
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data/"
)
