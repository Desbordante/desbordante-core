set(NAME ind.mind)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE mind.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::create_algo spdlog::spdlog_header_only magic_enum::magic_enum
                    Boost::headers
)
