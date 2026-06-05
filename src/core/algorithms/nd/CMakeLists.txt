add_subdirectory(nd_verifier)

set(NAME nd)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE nd.cpp util/get_vertical_names.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config ${DESBORDANTE_PREFIX}::model::table
    PRIVATE ${DESBORDANTE_PREFIX}::util Boost::headers
)
