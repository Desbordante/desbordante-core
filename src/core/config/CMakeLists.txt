file(GLOB_RECURSE CONDITIONS_SRCS conditions/*.cpp)

set(NAME config.enum_members_string)
desbordante_add_lib(NAME INTERFACE)
target_sources(${NAME} INTERFACE FILE_SET HEADERS FILES enum_members_string.h)
target_link_libraries(${NAME} INTERFACE magic_enum::magic_enum)

set(NAME config)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE ar_minimum_conf/option.cpp
            ar_minimum_support/option.cpp
            ${CONDITIONS_SRCS}
            column_index/option.cpp
            column_index/validate_index.cpp
            custom_random_seed/option.cpp
            descriptions.cpp
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
            transactional_data/option.cpp
)
target_link_libraries(
    ${NAME} PRIVATE magic_enum::magic_enum Boost::headers
                    ${DESBORDANTE_PREFIX}::config::enum_members_string
)
