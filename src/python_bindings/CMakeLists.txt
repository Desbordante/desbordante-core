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

pybind11_add_module(${MODULE_NAME} bind_main_classes.cpp bindings.cpp)
target_link_libraries(
    ${MODULE_NAME}
    PRIVATE pybind11::module
            Desbordante::algos
            magic_enum::magic_enum
            spdlog::spdlog_header_only
            Boost::headers
            Desbordante.bindlib.util
            Desbordante.bindlib.data
)

# Uncomment to debug linker issues
#target_link_libraries(${MODULE_NAME} PRIVATE "-Wl,--no-undefined" pybind11::embed)

set(NAME bindlib.data)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE data/bind_data_types.cpp)
target_link_libraries(${NAME} PRIVATE pybind11::pybind11)
set_target_properties(${NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")

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
)
target_link_libraries(
    ${NAME} PRIVATE magic_enum::magic_enum spdlog::spdlog_header_only Boost::headers pybind11::pybind11
)
set_target_properties(${NAME} PROPERTIES CXX_VISIBILITY_PRESET "hidden")

desbordante_add_bind(
    ac
    SRCS
    ac/bind_ac.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ac
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    afd_metric SRCS afd_metric/bind_afd_metric_calculation.cpp LIBS
    ${DESBORDANTE_PREFIX}::fd::afd_metric Boost::headers
)

desbordante_add_bind(
    ar
    SRCS
    ar/bind_ar.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ar
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    cfd
    SRCS
    cfd/bind_cfd.cpp
    cfd/bind_cfd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::cfd
    ${DESBORDANTE_PREFIX}::cfd::verifier
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    dc
    SRCS
    dc/bind_dc_verification.cpp
    dc/bind_fastadc.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::dc::verifier
    ${DESBORDANTE_PREFIX}::dc::fastadc
    magic_enum::magic_enum
    ${DESBORDANTE_PREFIX}::bindlib::util
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    dd
    SRCS
    dd/bind_dd_verification.cpp
    dd/bind_split.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::dd::verifier
    ${DESBORDANTE_PREFIX}::dd::split
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    fd
    SRCS
    fd/bind_fd.cpp
    fd/bind_fd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd::verifier
    ${DESBORDANTE_PREFIX}::fd::hy
    ${DESBORDANTE_PREFIX}::fd::aid
    ${DESBORDANTE_PREFIX}::fd::euler
    ${DESBORDANTE_PREFIX}::fd::depminer
    ${DESBORDANTE_PREFIX}::fd::dfd
    ${DESBORDANTE_PREFIX}::fd::fastfds
    ${DESBORDANTE_PREFIX}::fd::fdep
    ${DESBORDANTE_PREFIX}::fd::mine
    ${DESBORDANTE_PREFIX}::fd::fun
    ${DESBORDANTE_PREFIX}::fd::pyro
    ${DESBORDANTE_PREFIX}::fd::tane
    ${DESBORDANTE_PREFIX}::fd::tane::pfd
    ${DESBORDANTE_PREFIX}::bindlib::util
    magic_enum::magic_enum
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    fd.verifier.dynamic
    SRCS
    dynamic/bind_dynamic_fd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd::verifier::dynamic
    magic_enum::magic_enum
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    gfd
    SRCS
    gfd/bind_gfd.cpp
    gfd/bind_gfd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::gfd::miner
    ${DESBORDANTE_PREFIX}::gfd::validator
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    gspan
    SRCS
    gspan/bind_gspan.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fsm::gspan
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    ind
    SRCS
    ind/bind_ind.cpp
    ind/bind_ind_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ind::verifier
    ${DESBORDANTE_PREFIX}::ind::spider
    ${DESBORDANTE_PREFIX}::ind::faida
    ${DESBORDANTE_PREFIX}::ind::mind
    Boost::headers
)

desbordante_add_bind(
    md
    SRCS
    md/bind_md.cpp
    md/bind_md_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::md::verifier
    ${DESBORDANTE_PREFIX}::bindlib::util
    magic_enum::magic_enum
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    mfd
    SRCS
    mfd/bind_mfd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::metric::verifier
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    nar
    SRCS
    nar/bind_nar.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::nar::des
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    nd
    SRCS
    nd/bind_nd.cpp
    nd/bind_nd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::nd::verifier
    ${DESBORDANTE_PREFIX}::nd
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    od
    SRCS
    od/bind_od.cpp
    od/bind_od_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::od::fast
    ${DESBORDANTE_PREFIX}::od::order
    ${DESBORDANTE_PREFIX}::od::set_based_aod_verifier
    magic_enum::magic_enum
    spdlog::spdlog_header_only
    Boost::headers
)

desbordante_add_bind(
    pfd
    SRCS
    pfd/bind_pfd_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd::pfd_verifier
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    sfd
    SRCS
    sfd/bind_sfd.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd::sfd::cords
    ${DESBORDANTE_PREFIX}::bindlib::util
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    statistics
    SRCS
    statistics/bind_statistics.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::datastats
    magic_enum::magic_enum
    Boost::headers
)

desbordante_add_bind(
    ucc
    SRCS
    ucc/bind_ucc.cpp
    ucc/bind_ucc_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ucc::hpivalid
    ${DESBORDANTE_PREFIX}::ucc::hy
    ${DESBORDANTE_PREFIX}::ucc::pyro
    ${DESBORDANTE_PREFIX}::ucc::verifier
    ${DESBORDANTE_PREFIX}::bindlib::util
    magic_enum::magic_enum
    Boost::headers
)

# ---- Installation Setup ----

if(DESBORDANTE_BINDINGS STREQUAL "INSTALL")
    # Install Python bindings module to system
    # Places the compiled library in the root installation directory
    install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION .)
endif()
