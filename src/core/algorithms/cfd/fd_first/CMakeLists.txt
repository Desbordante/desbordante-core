set(NAME cfd.fdfirst)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE fd_first_algorithm.cpp model/partition_tidlist.cpp util/partition_util.cpp
                    util/prefix_tree.cpp
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::cfd magic_enum::magic_enum
    PRIVATE ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only
            Boost::headers
)
