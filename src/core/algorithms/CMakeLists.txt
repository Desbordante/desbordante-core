set(SUBDIRS
    algebraic_constraints
    association_rules
    cfd
    dc
    dd
    fd
    gfd
    ind
    md
    metric
    nar
    nd
    od/fastod
    od/order
    od/set_based_verifier
    statistics
    ucc
)

foreach(dir IN LISTS SUBDIRS)
    add_subdirectory(${dir})
endforeach()

set(NAME create_algo)
desbordante_add_lib(NAME INTERFACE)
target_sources(
    ${NAME}
    PUBLIC FILE_SET
           HEADERS
           FILES
           algorithms.h
           algorithm_types.h
           create_algorithm.h
           BASE_DIRS
           "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    INTERFACE ${DESBORDANTE_PREFIX}::ac
              ${DESBORDANTE_PREFIX}::ar
              ${DESBORDANTE_PREFIX}::cfd
              ${DESBORDANTE_PREFIX}::cfd::verifier
              ${DESBORDANTE_PREFIX}::datastats
              ${DESBORDANTE_PREFIX}::dd::split
              ${DESBORDANTE_PREFIX}::fd::aid
              ${DESBORDANTE_PREFIX}::fd::depminer
              ${DESBORDANTE_PREFIX}::fd::dfd
              ${DESBORDANTE_PREFIX}::fd::euler
              ${DESBORDANTE_PREFIX}::fd::fastfds
              ${DESBORDANTE_PREFIX}::fd::fdep
              ${DESBORDANTE_PREFIX}::fd::fun
              ${DESBORDANTE_PREFIX}::fd::hy
              ${DESBORDANTE_PREFIX}::fd::mine
              ${DESBORDANTE_PREFIX}::fd::pfd_verifier
              ${DESBORDANTE_PREFIX}::fd::pyro
              ${DESBORDANTE_PREFIX}::fd::sfd::cords
              ${DESBORDANTE_PREFIX}::fd::tane
              ${DESBORDANTE_PREFIX}::fd::tane::pfd
              ${DESBORDANTE_PREFIX}::fd::verifier
              ${DESBORDANTE_PREFIX}::gfd::validator
              ${DESBORDANTE_PREFIX}::ind::faida
              ${DESBORDANTE_PREFIX}::ind::spider
              ${DESBORDANTE_PREFIX}::ind::verifier
              ${DESBORDANTE_PREFIX}::metric::verifier
              ${DESBORDANTE_PREFIX}::md::hy
              ${DESBORDANTE_PREFIX}::nar::des
              ${DESBORDANTE_PREFIX}::od::fast
              ${DESBORDANTE_PREFIX}::od::order
              ${DESBORDANTE_PREFIX}::ucc::hy
              ${DESBORDANTE_PREFIX}::ucc::pyro
              ${DESBORDANTE_PREFIX}::ucc::hpivalid
              ${DESBORDANTE_PREFIX}::ucc::verifier
)

set(NAME algos)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE algo_factory.cpp algorithm.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::util ${DESBORDANTE_PREFIX}::config
            ${DESBORDANTE_PREFIX}::parser::csv spdlog::spdlog_header_only better-enums
            Boost::headers
)
