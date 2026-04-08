set(NAME dc.fastadc.structs)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE misc/typed_column_data_value_differences.cpp
            model/column_operand.cpp
            model/operator.cpp
            model/pli_shard.cpp
            model/predicate.cpp
            model/predicate_set.cpp
            providers/index_provider.cpp
            util/clue_set_builder.cpp
            util/cross_clue_set_builder.cpp
            util/dc_candidate_trie.cpp
            util/denial_constraint_set.cpp
            util/evidence_aux_structures_builder.cpp
            util/predicate_builder.cpp
            util/single_clue_set_builder.cpp
)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only frozen better-enums Boost::headers)

set(NAME dc.fastadc)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fastadc.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::dc::fastadc::structs spdlog::spdlog_header_only
                    better-enums frozen Boost::headers
)
