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
