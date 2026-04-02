set(SUBDIRS
    algebraic_constraints
    ar
    cfd
    cind
    dc
    dd
    fd
    fem
    fsm
    gdd
    gfd
    ind
    md
    metric
    nar
    nd
    od/fastod
    od/order
    od/set_based_verifier
    pac
    statistics
    ucc
    sd
)

foreach(dir IN LISTS SUBDIRS)
    add_subdirectory(${dir})
endforeach()

set(NAME algos)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE algo_factory.cpp algorithm.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::model::types Boost::headers
    PRIVATE ${DESBORDANTE_PREFIX}::util ${DESBORDANTE_PREFIX}::config
            ${DESBORDANTE_PREFIX}::parser::csv ${DESBORDANTE_PREFIX}::parser::sequence
            ${DESBORDANTE_PREFIX}::model::sequence spdlog::spdlog_header_only
)
