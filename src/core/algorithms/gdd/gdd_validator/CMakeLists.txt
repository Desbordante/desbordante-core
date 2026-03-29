set(NAME gdd.validator)
desbordante_add_lib(NAME)
target_sources(
        ${NAME} PRIVATE gdd_validator.cpp naive_gdd_validator.cpp
)
target_link_libraries(
        ${NAME}
        PRIVATE ${DESBORDANTE_PREFIX}::parser::graph spdlog::spdlog_header_only
        ${DESBORDANTE_PREFIX}::gdd ${DESBORDANTE_PREFIX}::algos better-enums Boost::headers
)
