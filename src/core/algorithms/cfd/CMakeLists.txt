add_subdirectory(cfd_verifier)
add_subdirectory(fd_first)

set(NAME cfd.model)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE model/cfd_relation_data.cpp model/raw_cfd.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table spdlog::spdlog_header_only Boost::headers
)

set(NAME cfd)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cfd_discovery.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::model ${DESBORDANTE_PREFIX}::algos
            ${DESBORDANTE_PREFIX}::util spdlog::spdlog_header_only Boost::headers
)
