add_custom_target(copy-python-examples ALL)
add_custom_command(TARGET copy-python-examples
        COMMAND ${CMAKE_COMMAND} -E
            make_directory ${CMAKE_BINARY_DIR}/examples
                           ${CMAKE_BINARY_DIR}/examples/datasets
        COMMAND ${CMAKE_COMMAND} -E
            copy ${CMAKE_SOURCE_DIR}/examples/dedupe.py
                 ${CMAKE_SOURCE_DIR}/examples/mine_typos.py
                 ${CMAKE_SOURCE_DIR}/examples/anomaly_detection.py
                 ${CMAKE_BINARY_DIR}/examples
        COMMAND ${CMAKE_COMMAND} -E
            copy ${CMAKE_SOURCE_DIR}/examples/datasets/duplicates.csv
                 ${CMAKE_SOURCE_DIR}/examples/datasets/Workshop.csv
                 ${CMAKE_SOURCE_DIR}/examples/datasets/cargo_data_1.csv
                 ${CMAKE_SOURCE_DIR}/examples/datasets/cargo_data_2.csv
                 ${CMAKE_SOURCE_DIR}/examples/datasets/cargo_data_3.csv
                 ${CMAKE_BINARY_DIR}/examples/datasets
)
