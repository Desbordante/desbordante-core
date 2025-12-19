# ---- Dataset pulling and extracting----
include(../cmake/desbordante_configure_datasets.cmake)

# Create input data directory (safe for incremental builds)
set(INPUT_DATA_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data")
file(MAKE_DIRECTORY ${INPUT_DATA_DIR})

# Declare datasets and their SHA256 sums. These sums must be manually updated with each change of
# datasets in `desbordante-data`
set(DATASETS datasets highway-neighbors)
set(DATASETS_SHA2_datasets f5d93344a5bddad20333e447909b3f9c9644dd6dddcd56500ed2a14e8c1d7351)
set(DATASETS_SHA2_highway-neighbors
    7cfeb6387773309ea6ff6dfe6629d44cb35c460f29c25b0e1f88bfe99d7312c9
)
if(COMPILE_BENCHMARKS)
    list(APPEND DATASETS performance-tests)
    set(DATASETS_SHA2_performance-tests
        3b1d6591dfaabfc8ceeecb123b2237e346d870224a1e0dd05ca238275dbd3ce2
    )
endif()

message(STATUS "Fetch test/benchmark datasets")
foreach(name IN LISTS DATASETS)
    desbordante_configure_datasets(
        STAMP_FILE
        NAME
        ${name}
        SHA2
        ${DATASETS_SHA2_${name}}
        DOWNLOAD_DIR
        ${CMAKE_CURRENT_LIST_DIR}
        EXTRACT_DIR
        ${INPUT_DATA_DIR}
    )
    add_custom_target(Desbordante.zip.${name} DEPENDS ${STAMP_FILE})
endforeach()

# ---- Test Data Preparation ----

# Copy test datasets to build output directory before any build target
add_custom_target(
    Desbordante.copy_test_input_data
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/test_input_data
            ${INPUT_DATA_DIR}
    COMMENT "Copy new/modified test datasets to build directory"
)

# ---- Build Cleanup Configuration ----

# Register test data directory for automatic removal during cleanup
# Ensures test artifacts are deleted with `cmake --build . --target clean`
set_property(
    DIRECTORY
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${INPUT_DATA_DIR}
)
