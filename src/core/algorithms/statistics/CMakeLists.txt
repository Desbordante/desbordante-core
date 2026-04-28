set(NAME datastats)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE data_stats.cpp statistic.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::model::types
                    ${DESBORDANTE_PREFIX}::algos magic_enum::magic_enum Boost::headers
)
