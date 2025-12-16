# ---- Dataset pulling and extracting----
include(desbordante_configure_datasets)

# Create input data directory (safe for incremental builds)
set(INPUT_DATA_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data")
file(MAKE_DIRECTORY ${INPUT_DATA_DIR})

# Declare datasets names from `desbordante-data`
set(DATASETS datasets highway-neighbors)
if(DESBORDANTE_BUILD_BENCHMARKS)
    list(APPEND DATASETS performance-tests)
endif()

if(DESBORDANTE_FETCH_DATASETS)
    set(MESSAGE "Fetching test datasets")
    if(DESBORDANTE_BUILD_BENCHMARKS)
        set(MESSAGE "Fetching test and benchmark datasets")
    endif()
    message(STATUS ${MESSAGE})
endif()

foreach(name IN LISTS DATASETS)
    if(DESBORDANTE_FETCH_DATASETS)
        desbordante_fetch_datasets(NAME ${name} DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR})
    else()
        message(WARNING "There could be updates in dataset ${name} (Fetch disabled)")
    endif()
    set(FILE_PATH "${CMAKE_CURRENT_LIST_DIR}/${name}.zip")
    if(NOT EXISTS ${FILE_PATH})
        message(FATAL_ERROR "Cannot access ${FILE_PATH}: no such file. Datasets are required for \
        unit tests or benchmarks."
        )
    endif()
    desbordante_unpack_datasets(STAMP_FILE PATH ${FILE_PATH} EXTRACT_DIR ${INPUT_DATA_DIR})
    add_custom_target(Desbordante.zip.${name} DEPENDS ${STAMP_FILE})
endforeach()

# ---- Test Data Preparation ----

# Copy test datasets to build output directory before any build target
add_custom_target(
    Desbordante.copy_test_input_data
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/test_input_data
            ${INPUT_DATA_DIR}
    COMMENT "Copying new/modified test datasets to build directory"
)

# ---- Build Cleanup Configuration ----

# Register test data directory for automatic removal during cleanup
# Ensures test artifacts are deleted with `cmake --build . --target clean`
set_property(
    DIRECTORY
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${INPUT_DATA_DIR}
)
