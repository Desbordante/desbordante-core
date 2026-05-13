set(NAME dd.fastdd.structs)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/pli_shard.cpp
            trees/minimize_tree.cpp
            trees/translating_tree_search.cpp
            util/differential_function_builder.cpp
            util/isn_info.cpp
            util/threshold_calculator.cpp
)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only Boost::headers)

set(NAME dd.fastdd)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE fastdd.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::dd::fastdd::structs
            ${DESBORDANTE_PREFIX}::dd
            ${DESBORDANTE_PREFIX}::config
            ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            spdlog::spdlog_header_only
            ${DESBORDANTE_PREFIX}::algos
            magic_enum::magic_enum
            Boost::headers
)

set(NAME dd.fastdd_run)
add_executable(${NAME})
set_target_properties(${NAME} PROPERTIES LINK_LIBRARIES_ONLY_TARGETS ON)
target_sources(${NAME} PRIVATE main.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::dd::fastdd spdlog::spdlog_header_only
                    magic_enum::magic_enum Boost::headers
)
