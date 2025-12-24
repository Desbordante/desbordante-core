cmake_minimum_required(VERSION 3.25)

project(
    Desbordante.bindings
    LANGUAGES CXX
    DESCRIPTION "Python bindings to the interface of the Desbordante C++ core library"
)

CPMAddPackage("gh:pybind/pybind11@2.13.4")

# ---- Python Bindings Configuration ----

# Define the output name for Python bindings module
# NOTE: Matches the Python import name (e.g. 'import desbordante')
set(MODULE_NAME
    "desbordante"
    CACHE INTERNAL ""
)

add_library(${MODULE_NAME} MODULE bind_main_classes.cpp bind_main_classes.h bindings.cpp)
target_link_libraries(
    ${MODULE_NAME}
    PRIVATE pybind11::module
            Desbordante::algos
            better-enums
            spdlog::spdlog_header_only
            Boost::headers
            Desbordante.bindlib.util
            Desbordante.bindlib.data
)
pybind11_extension(${MODULE_NAME})

if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug|RelWithDebInfo)
    # Strip unnecessary sections of the binary on Linux/macOS
    pybind11_strip(${MODULE_NAME})
endif()

set_target_properties(
    ${MODULE_NAME}
    PROPERTIES CXX_VISIBILITY_PRESET "hidden"
               # Configure output directory to central build folder
               # Ensures generated .so/.pyd files are placed alongside other binaries
               LIBRARY_OUTPUT_DIRECTORY
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} # ${CMAKE_BINARY_DIR}/target
)

# Uncomment to debug linker issues
#target_link_libraries(${MODULE_NAME} PRIVATE "-Wl,--no-undefined" pybind11::embed)

set(NAME bindlib.data)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE data/bind_data_types.h data/bind_data_types.cpp
    PRIVATE FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(${NAME} PRIVATE pybind11::pybind11)

set(NAME bindlib.util)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE py_util/create_dataframe_reader.cpp
            py_util/dataframe_reader.cpp
            py_util/get_py_type.cpp
            py_util/logging.cpp
            py_util/opt_to_py.cpp
            py_util/py_to_any.cpp
            py_util/table_serialization.cpp
    PRIVATE FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_link_libraries(
    ${NAME} PRIVATE better-enums spdlog::spdlog_header_only Boost::headers pybind11::pybind11
)

desbordante_add_bind(
    ac
    SRCS
    ac/bind_ac.h
    ac/bind_ac.cpp
    LIBS
    Desbordante::ac
    better-enums
    Boost::headers
)

desbordante_add_bind(
    ar
    SRCS
    ar/bind_ar.h
    ar/bind_ar.cpp
    LIBS
    Desbordante::ar
    better-enums
    Boost::headers
)

desbordante_add_bind(
    cfd
    SRCS
    cfd/bind_cfd.h
    cfd/bind_cfd_verification.h
    cfd/bind_cfd.cpp
    cfd/bind_cfd_verification.cpp
    LIBS
    Desbordante::cfd
    Desbordante::cfd::verifier
    better-enums
    Boost::headers
)

desbordante_add_bind(
    dc
    SRCS
    dc/bind_dc_verification.h
    dc/bind_fastadc.h
    dc/bind_dc_verification.cpp
    dc/bind_fastadc.cpp
    LIBS
    Desbordante::dc::verifier
    Desbordante::dc::fastadc
    frozen
    better-enums
    Desbordante.bindlib.util
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    dd
    SRCS
    dd/bind_dd_verification.h
    dd/bind_split.h
    dd/bind_dd_verification.cpp
    dd/bind_split.cpp
    LIBS
    Desbordante::dd::verifier
    Desbordante::dd::split
    better-enums
    Boost::headers
)

desbordante_add_bind(
    fd
    SRCS
    fd/bind_fd.h
    fd/bind_fd_verification.h
    fd/bind_fd.cpp
    fd/bind_fd_verification.cpp
    LIBS
    Desbordante::fd::verifier
    Desbordante::fd::hy
    Desbordante::fd::aid
    Desbordante::fd::euler
    Desbordante::fd::depminer
    Desbordante::fd::dfd
    Desbordante::fd::fastfds
    Desbordante::fd::fdep
    Desbordante::fd::mine
    Desbordante::fd::fun
    Desbordante::fd::pyro
    Desbordante::fd::tane
    Desbordante::fd::tane::pfd
    Desbordante.bindlib.util
    better-enums
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    fd.verifier.dynamic
    SRCS
    dynamic/bind_dynamic_fd_verification.h
    dynamic/bind_dynamic_fd_verification.cpp
    LIBS
    Desbordante::fd::verifier::dynamic
    better-enums
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    gfd
    SRCS
    gfd/bind_gfd.h
    gfd/bind_gfd_verification.h
    gfd/bind_gfd.cpp
    gfd/bind_gfd_verification.cpp
    LIBS
    Desbordante::gfd::miner
    Desbordante::gfd::validator
    better-enums
    Boost::headers
)

desbordante_add_bind(
    ind
    SRCS
    ind/bind_ind.h
    ind/bind_ind_verification.h
    ind/bind_ind.cpp
    ind/bind_ind_verification.cpp
    LIBS
    Desbordante::ind::verifier
    Desbordante::ind::spider
    Desbordante::ind::faida
    Desbordante::ind::mind
    Boost::headers
)

desbordante_add_bind(
    md
    SRCS
    md/bind_md.h
    md/bind_md_verification.h
    md/object_similarity_measure.h
    md/bind_md.cpp
    md/bind_md_verification.cpp
    LIBS
    Desbordante::md::verifier
    Desbordante.bindlib.util
    better-enums
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    mfd
    SRCS
    mfd/bind_mfd_verification.h
    mfd/bind_mfd_verification.cpp
    LIBS
    Desbordante::metric::verifier
    better-enums
    Boost::headers
)

desbordante_add_bind(
    nar
    SRCS
    nar/bind_nar.cpp
    nar/bind_nar.h
    LIBS
    Desbordante::nar::des
    better-enums
    Boost::headers
)

desbordante_add_bind(
    nd
    SRCS
    nd/bind_nd.h
    nd/bind_nd_verification.h
    nd/bind_nd.cpp
    nd/bind_nd_verification.cpp
    LIBS
    Desbordante::nd::verifier
    Desbordante::nd
    better-enums
    Boost::headers
)

desbordante_add_bind(
    od
    SRCS
    od/bind_od.h
    od/bind_od_verification.h
    od/bind_od.cpp
    od/bind_od_verification.cpp
    LIBS
    Desbordante::od::fast
    Desbordante::od::order
    Desbordante::od::set_based_aod_verifier
    better-enums
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    pfd
    SRCS
    pfd/bind_pfd_verification.h
    pfd/bind_pfd_verification.cpp
    LIBS
    Desbordante::fd::pfd_verifier
    better-enums
    Boost::headers
)

desbordante_add_bind(
    sfd
    SRCS
    sfd/bind_sfd.h
    sfd/bind_sfd.cpp
    LIBS
    Desbordante::fd::sfd::cords
    Desbordante.bindlib.util
    better-enums
    Boost::headers
)

desbordante_add_bind(
    statistics
    SRCS
    statistics/bind_statistics.h
    statistics/bind_statistics.cpp
    LIBS
    Desbordante::datastats
    better-enums
    Boost::headers
)

desbordante_add_bind(
    ucc
    SRCS
    ucc/bind_ucc.h
    ucc/bind_ucc_verification.h
    ucc/bind_ucc.cpp
    ucc/bind_ucc_verification.cpp
    LIBS
    Desbordante::ucc::hpivalid
    Desbordante::ucc::hy
    Desbordante::ucc::pyro
    Desbordante::ucc::verifier
    Desbordante.bindlib.util
    better-enums
    Boost::headers
)

# ---- Installation Setup ----

if(DESBORDANTE_BINDINGS STREQUAL "INSTALL")
    # Install Python bindings module to system
    # Places the compiled library in the root installation directory
    install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION .)
endif()
