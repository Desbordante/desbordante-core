set(NAME od.order)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE dependency_checker.cpp list_lattice.cpp order.cpp order_utility.cpp
            sorted_partitions.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only
                    ${DESBORDANTE_PREFIX}::algos better-enums Boost::headers
)
