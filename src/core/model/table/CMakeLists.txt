set(NAME model.table)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE agree_set_factory.cpp
            block_data.cpp
            column_combination.cpp
            column.cpp
            column_domain.cpp
            column_domain_iterator.cpp
            column_layout_relation_data.cpp
            column_layout_typed_relation_data.cpp
            dynamic_position_list_index.cpp
            identifier_set.cpp
            position_list_index.cpp
            relational_schema.cpp
            typed_column_data.cpp
            vertical.cpp
            vertical_map.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::types spdlog::spdlog_header_only better-enums
                    Boost::headers ${DESBORDANTE_PREFIX}::algos
)
