set(NAME od.fast)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE fastod.cpp
            model/attribute_pair.cpp
            model/attribute_set.cpp
            model/canonical_od.cpp
            partitions/complex_stripped_partition.cpp
            partitions/range_based_stripped_partition.cpp
            partitions/stripped_partition.cpp
            storage/data_frame.cpp
            util/timer.cpp
            util/type_util.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only
                    ${DESBORDANTE_PREFIX}::algos better-enums Boost::headers
)
