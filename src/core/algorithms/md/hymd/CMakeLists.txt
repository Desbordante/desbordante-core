set(NAME md.hy)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE lattice/cardinality/min_picker_lattice.cpp
            lattice/cardinality/min_picking_level_getter.cpp
            lattice/cardinality/one_by_one_min_picker.cpp
            lattice/md_lattice.cpp
            hymd.cpp
            lattice_traverser.cpp
            record_pair_inferrer.cpp
            similarity_data.cpp
            validator.cpp
)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::md::hy::preprocessing ${DESBORDANTE_PREFIX}::md
                    spdlog::spdlog_header_only Boost::headers
)

set(NAME md.hy.preprocessing)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE indexes/dictionary_compressor.cpp
            indexes/keyed_position_list_index.cpp
            indexes/records_info.cpp
            indexes/similarity_index.cpp
            preprocessing/column_matches/jaccard.cpp
            preprocessing/column_matches/lcs.cpp
            preprocessing/column_matches/levenshtein.cpp
            preprocessing/column_matches/monge_elkan.cpp
            preprocessing/column_matches/smith_waterman_gotoh.cpp
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only magic_enum::magic_enum
                    Boost::headers
)
