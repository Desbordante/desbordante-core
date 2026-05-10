set(NAME ac)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ac_algorithm.cpp ac_exception_finder.cpp)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::model::types
            ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config spdlog::spdlog_header_only
            Boost::headers
)
