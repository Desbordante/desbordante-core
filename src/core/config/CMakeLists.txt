set(NAME config)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE column_index/option.cpp
            column_index/validate_index.cpp
            custom_random_seed/option.cpp
            equal_nulls/option.cpp
            error/option.cpp
            error_measure/option.cpp
            indices/option.cpp
            max_arity/option.cpp
            max_lhs/option.cpp
            mem_limit/option.cpp
            tabular_data/crud_operations/delete/option.cpp
            tabular_data/crud_operations/insert/option.cpp
            tabular_data/crud_operations/update/option.cpp
            tabular_data/input_table/option.cpp
            tabular_data/input_tables/option.cpp
            thread_number/option.cpp
            time_limit/option.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE better-enums Boost::headers)
