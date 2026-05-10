set(SUBDIRS
    afd_metric
    aidfd
    depminer
    dfd
    eulerfd
    fastfds
    fd_mine
    fd_verifier
    fdep
    fun
    hycommon
    hyfd
    pfd_verifier
    pyro
    pyrocommon
    sfd
    tane
)

foreach(dir IN LISTS SUBDIRS)
    add_subdirectory(${dir})
endforeach()

# --- FD ---
set(NAME fd)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE fd.cpp fd_algorithm.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::algos Boost::headers
)

# --- PliBasedFD ---
set(NAME fd.pli)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE pli_based_fd_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd ${DESBORDANTE_PREFIX}::util Boost::headers
)
