set(NAME ucc.verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ucc_verifier.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table better-enums Boost::headers
)
