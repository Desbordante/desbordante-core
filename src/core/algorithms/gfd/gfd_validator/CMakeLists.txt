set(NAME gfd.validator)
desbordante_add_lib(NAME)
target_sources(
    ${NAME} PRIVATE balancer.cpp egfd_validator.cpp gfd_handler.cpp gfd_validator.cpp
                    naivegfd_validator.cpp
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::parser::graph spdlog::spdlog_header_only
            ${DESBORDANTE_PREFIX}::gfd ${DESBORDANTE_PREFIX}::algos magic_enum::magic_enum Boost::headers
)
