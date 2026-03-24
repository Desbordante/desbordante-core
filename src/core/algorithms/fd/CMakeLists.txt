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
    afd_metric
    tane
)

foreach(dir IN LISTS SUBDIRS)
    add_subdirectory(${dir})
endforeach()

# --- AFD ---
set(NAME afd)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE afd_algorithm.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::algos Boost::headers
)


# --- PliBasedAFD ---
set(NAME afd.pli)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE pli_based_afd_algorithm.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::afd Boost::headers)


# --- FD ---
set(NAME fd)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE fd.cpp fd_algorithm.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::afd ${DESBORDANTE_PREFIX}::algos Boost::headers
)

# --- PliBasedFD ---
set(NAME fd.pli)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE pli_based_fd_algorithm.cpp)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::fd Boost::headers)


