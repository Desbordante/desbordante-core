set(NAME ind.mind)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE mind.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::util
                    ${DESBORDANTE_PREFIX}::ind::spider spdlog::spdlog_header_only Boost::headers
)
