# Load CMake's built-in GoogleTest module
include(GoogleTest)

# --- Libs for tests ---
set(NAME testlib.main)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE main.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only gtest)

set(NAME testlib.gfd.paths)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE all_gfd_paths.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE Boost::headers)

set(NAME testlib.hash)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE test_hash_util.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

set(NAME testlib.ind)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE test_ind_util.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE ${DESBORDANTE_PREFIX}::ind gtest Boost::headers)

# --- AC ---
desbordante_add_test(
    ac
    SRCS
    test_ac_algorithm.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ac
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

# --- AR ---
desbordante_add_test(
    ar
    SRCS
    test_apriori.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ar
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)

# --- CFD ---
desbordante_add_test(
    cfd.algos
    SRCS
    test_cfd_algos.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::cfd
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::cfd::model
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

desbordante_add_test(
    cfd.relation_data
    SRCS
    test_cfd_relation_data.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::parser::csv
    ${DESBORDANTE_PREFIX}::cfd::util
    ${DESBORDANTE_PREFIX}::cfd::model
    Boost::headers
)
desbordante_add_test(
    cfd.verifier
    SRCS
    test_cfd_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::cfd::verifier
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

# --- CSV ---
desbordante_add_test(
    csv.parser
    SRCS
    test_csv_parser.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::parser::csv
    ${DESBORDANTE_PREFIX}::testlib::common
    gmock
)

# --- DataStats ---
desbordante_add_test(
    datastats
    SRCS
    test_data_stats.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::datastats
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)

# --- DC ---
desbordante_add_test(
    dc.structs
    SRCS
    test_dc_structures.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::dc::fastadc::structs
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::parser::csv
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::model::table
    spdlog::spdlog_header_only
    frozen
    better-enums
    gmock
    Boost::headers
)
desbordante_add_test(
    dc.verifier
    SRCS
    test_dc_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::dc::verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    frozen
    better-enums
    gmock
    Boost::headers
)

# --- DD ---
desbordante_add_test(
    dd.verifier
    SRCS
    test_dd_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::dd::verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    dd.split
    SRCS
    test_split.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::dd::split
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

# --- DES ---
desbordante_add_test(
    nar.des
    SRCS
    test_des.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::nar::des
    ${DESBORDANTE_PREFIX}::nar
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::algos
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

# --- FD ---
desbordante_add_test(
    fd.algos
    SRCS
    test_fd_algorithm.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::fd::depminer
    ${DESBORDANTE_PREFIX}::fd::dfd
    ${DESBORDANTE_PREFIX}::fd::fastfds
    ${DESBORDANTE_PREFIX}::fd::fdep
    ${DESBORDANTE_PREFIX}::fd::fun
    ${DESBORDANTE_PREFIX}::fd::hy
    ${DESBORDANTE_PREFIX}::fd::pyro
    ${DESBORDANTE_PREFIX}::fd::tane::pfd
    ${DESBORDANTE_PREFIX}::fd::tane
    ${DESBORDANTE_PREFIX}::fd
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)
desbordante_add_test(
    fd.approx
    SRCS
    test_fd_util.h
    test_fd_approximate.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::fd
    ${DESBORDANTE_PREFIX}::fd::euler
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)
desbordante_add_test(
    fd.mine
    SRCS
    test_fd_mine.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::fd
    ${DESBORDANTE_PREFIX}::fd::pyro
    ${DESBORDANTE_PREFIX}::fd::mine
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)
desbordante_add_test(
    fd.tane.pfd
    SRCS
    test_pfdtane.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd
    ${DESBORDANTE_PREFIX}::fd::tane::pfd
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    fd.pfd_verifier
    SRCS
    test_pfd_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd::pfd_verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::algos
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    fd.sfd.cords
    SRCS
    test_sfd.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd::sfd::cords
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    fd.tane
    SRCS
    test_tane_afd_measures.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::fd
    ${DESBORDANTE_PREFIX}::fd::tane
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    fd.verifier.dynamic
    SRCS
    test_dynamic_fd_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::fd::verifier
    ${DESBORDANTE_PREFIX}::fd::verifier::dynamic
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::algos
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    fd.verifier
    SRCS
    test_fd_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::fd::verifier
    ${DESBORDANTE_PREFIX}::algos
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

# --- GFD ---
desbordante_add_test(
    gfd.miner
    SRCS
    test_gfd_miner.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::gfd::miner
    ${DESBORDANTE_PREFIX}::testlib::gfd::paths
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)
desbordante_add_test(
    gfd.validator
    SRCS
    test_gfd_validator.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::gfd::validator
    ${DESBORDANTE_PREFIX}::testlib::gfd::paths
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
)

# --- IND ---
desbordante_add_test(
    ind.algos
    SRCS
    test_ind_algorithms.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::algos
    ${DESBORDANTE_PREFIX}::ind
    ${DESBORDANTE_PREFIX}::testlib::ind
    ${DESBORDANTE_PREFIX}::testlib::hash
    ${DESBORDANTE_PREFIX}::ind::spider
    ${DESBORDANTE_PREFIX}::ind::mind
    ${DESBORDANTE_PREFIX}::ind::faida
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    ind.faida
    SRCS
    test_faida.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::ind
    ${DESBORDANTE_PREFIX}::ind::faida
    ${DESBORDANTE_PREFIX}::testlib::ind
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)
desbordante_add_test(
    ind.verifier
    SRCS
    test_ind_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ind::verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::algos
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
)

# --- Model ---
desbordante_add_test(
    model.bitset SRCS test_bitset.cpp LIBS spdlog::spdlog_header_only Boost::headers
)
desbordante_add_test(
    model.types.numeric_type_cast SRCS test_numerictype_cast.cpp LIBS better-enums Boost::headers
)
desbordante_add_test(
    model.types.double_compare SRCS test_double_compare.cpp LIBS better-enums Boost::headers
)
desbordante_add_test(
    model.types
    SRCS
    test_types.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::util
    ${DESBORDANTE_PREFIX}::model::types
    better-enums
    gmock
    Boost::headers
)
desbordante_add_test(
    model.table
    SRCS
    test_typed_column_data.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::parser::csv
    better-enums
    Boost::headers
)

# --- ND ---
desbordante_add_test(
    nd.verifier
    SRCS
    test_nd_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::nd::verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::types
)

# --- MD ---
desbordante_add_test(
    md.hy
    SRCS
    test_hymd.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::md::hy
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
)
desbordante_add_test(
    md.metrics
    SRCS
    test_hymd_metrics.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::md::hy::preprocessing
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
)
desbordante_add_test(
    md.verifier
    SRCS
    test_md_verification.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::md::verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
)

# --- Metric ---
desbordante_add_test(
    metric.verifier
    SRCS
    test_metric_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::metric::verifier
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::types
)

# --- OD ---
desbordante_add_test(
    od.fastod
    SRCS
    test_fastod.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::od::fast
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::types
)
desbordante_add_test(
    od.order
    SRCS
    test_order.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::od::order
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::types
)
desbordante_add_test(
    od.set_based_aod_verifier
    SRCS
    test_set_based_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::od::set_based_aod_verifier
    ${DESBORDANTE_PREFIX}::od::fast
    ${DESBORDANTE_PREFIX}::testlib::common
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::types
)

# --- UCC ---
desbordante_add_test(
    ucc.algos
    SRCS
    test_ucc_algorithms.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::ucc
    ${DESBORDANTE_PREFIX}::ucc::hy
    ${DESBORDANTE_PREFIX}::testlib::hash
    ${DESBORDANTE_PREFIX}::ucc::pyro
    ${DESBORDANTE_PREFIX}::ucc::hpivalid
    spdlog::spdlog_header_only
    better-enums
    gmock
    Boost::headers
    ${DESBORDANTE_PREFIX}::fd::hy::model
)
desbordante_add_test(
    ucc.verifier
    SRCS
    test_ucc_verifier.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::ucc::verifier
    ${DESBORDANTE_PREFIX}::ucc::hy
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::model::table
    spdlog::spdlog_header_only
    better-enums
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::types
    ${DESBORDANTE_PREFIX}::fd::hy::model
)

# --- Util ---
desbordante_add_test(
    util
    SRCS
    test_util.cpp
    LIBS
    ${DESBORDANTE_PREFIX}::testlib::common
    ${DESBORDANTE_PREFIX}::fd::pyro::common
    ${DESBORDANTE_PREFIX}::parser::csv
    gmock
    Boost::headers
    ${DESBORDANTE_PREFIX}::model::table
    ${DESBORDANTE_PREFIX}::model::types
)
