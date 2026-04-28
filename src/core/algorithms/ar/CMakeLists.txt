add_subdirectory(apriori)
add_subdirectory(ar_verifier)

set(NAME ar)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE ar_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::transaction ${DESBORDANTE_PREFIX}::algos
                    spdlog::spdlog_header_only better-enums Boost::headers
)
