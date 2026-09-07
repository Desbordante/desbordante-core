set(SUBDIRS
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
    afd_metric
)

foreach(dir IN LISTS SUBDIRS)
    add_subdirectory(${dir})
endforeach()

# --- AFD ---
set(NAME afd)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE afd_algorithm.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::fd
    PRIVATE ${DESBORDANTE_PREFIX}::model::table Boost::headers
)

# --- PliBasedAFD ---
set(NAME afd.pli)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE pli_based_afd_algorithm.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::afd
    PRIVATE Boost::headers
)

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

# --- LoadDataInternal is creating PLIs with probing table ---
# Temporary, avoid using unless you're certain there is no better way than the one exactly in this class already.
set(NAME fd.probing_tables_load_data)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE probing_tables_load_data.cpp)
target_link_libraries(
    ${NAME} PUBLIC ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::algos
)
