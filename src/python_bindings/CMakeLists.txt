SET(BINDINGS_NAME desbordante)

file(GLOB_RECURSE sources "*.h*" "*.cpp*")
pybind11_add_module(${BINDINGS_NAME} ${sources})

target_link_libraries(${BINDINGS_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_lib)

add_custom_command(TARGET ${BINDINGS_NAME}
        COMMAND ${CMAKE_COMMAND} -E
            copy ${CMAKE_SOURCE_DIR}/src/python_bindings/test_bindings.py
                 ${CMAKE_SOURCE_DIR}/test_input_data/WDC_satellites.csv
                 ${CMAKE_BINARY_DIR}/src/python_bindings/
)

add_custom_target(copy-python-examples ALL)
add_custom_command(TARGET copy-python-examples
        COMMAND ${CMAKE_COMMAND} -E
            make_directory ${CMAKE_BINARY_DIR}/src/python_bindings/examples
                           ${CMAKE_BINARY_DIR}/src/python_bindings/examples/datasets
        COMMAND ${CMAKE_COMMAND} -E
            copy ${CMAKE_SOURCE_DIR}/src/python_bindings/examples/dedupe.py
                 ${CMAKE_SOURCE_DIR}/src/python_bindings/examples/mine_typos.py
                 ${CMAKE_BINARY_DIR}/src/python_bindings/examples
        COMMAND ${CMAKE_COMMAND} -E
            copy ${CMAKE_SOURCE_DIR}/src/python_bindings/examples/datasets/duplicates.csv
                 ${CMAKE_SOURCE_DIR}/test_input_data/Workshop.csv
                 ${CMAKE_BINARY_DIR}/src/python_bindings/examples/datasets
)
