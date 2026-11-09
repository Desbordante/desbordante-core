set(NAME cfd.cfun)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cfun.cpp quadruple.cpp attribute_utils.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config
           ${DESBORDANTE_PREFIX}::cfd::model ${DESBORDANTE_PREFIX}::fd Boost::headers
    PRIVATE ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only
)
