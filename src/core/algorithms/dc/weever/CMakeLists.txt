set(NAME dc.weever)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE weever.cpp)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            ${DESBORDANTE_PREFIX}::dc::model
            ${DESBORDANTE_PREFIX}::dc::parser
            spdlog::spdlog_header_only
            frozen
            better-enums
            roaring
            Boost::headers
)
