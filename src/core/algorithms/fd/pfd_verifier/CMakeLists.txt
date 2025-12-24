set(NAME fd.pfd_verifier)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE pfd_verifier.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table magic_enum::magic_enum Boost::headers
)
