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
            position_list_index_with_singletons.cpp
            relational_schema.cpp
            typed_column_data.cpp
            vertical.cpp
            vertical_map.cpp
)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::types spdlog::spdlog_header_only Boost::headers
                    ${DESBORDANTE_PREFIX}::algos
)
