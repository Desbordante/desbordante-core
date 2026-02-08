add_subdirectory(cfd_verifier)

set(NAME cfd.util)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE util/cfd_output_util.cpp util/partition_tidlist_util.cpp util/partition_util.cpp
            util/set_util.cpp util/tidlist_util.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE Boost::headers)

set(NAME cfd.model)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/cfd_relation_data.cpp model/partition_tidlist.cpp model/raw_cfd.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::cfd::util ${DESBORDANTE_PREFIX}::model::table
                    Boost::headers
)

set(NAME cfd)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE cfd_discovery.cpp fd_first_algorithm.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::model ${DESBORDANTE_PREFIX}::cfd::util
            ${DESBORDANTE_PREFIX}::algos spdlog::spdlog_header_only better-enums Boost::headers
)
