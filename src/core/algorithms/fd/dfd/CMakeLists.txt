set(NAME fd.dfd)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE dfd.cpp
            column_order/column_order.cpp
            column_order/ordered_partition.cpp
            lattice_observations/lattice_observations.cpp
            lattice_traversal/lattice_traversal.cpp
            partition_storage/partition_storage.cpp
            pruning_maps/dependencies_map.cpp
            pruning_maps/non_dependencies_map.cpp
            pruning_maps/pruning_map.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd::pli spdlog::spdlog_header_only Boost::headers
)
