set(NAME ind.faida)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE candidate_generation/apriori_candidate_generator.cpp
            hashing/murmur_hash_3.cpp
            inclusion_testing/combined_inclusion_tester.cpp
            inclusion_testing/sampled_inverted_index.cpp
            preprocessing/abstract_column_store.cpp
            preprocessing/hashed_column_store.cpp
            preprocessing/preprocessor.cpp
            util/simple_ind.cpp
            faida.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE spdlog::spdlog_header_only
            ${DESBORDANTE_PREFIX}::ind
            emhash
            better-enums
            atomic_bitvector
            Boost::headers
            ${DESBORDANTE_PREFIX}::model::table
)
