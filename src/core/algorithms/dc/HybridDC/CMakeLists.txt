set(NAME dc.hybriddc)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE hybrid_dc.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::dc::fastadc::structs spdlog::spdlog_header_only
                    better-enums frozen Boost::headers
)
