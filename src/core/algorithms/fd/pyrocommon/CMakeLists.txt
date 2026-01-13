set(NAME fd.pyro.common)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE core/dependency_candidate.cpp
            core/dependency_consumer.cpp
            core/dependency_strategy.cpp
            core/fd_g1_strategy.cpp
            core/key_g1_strategy.cpp
            core/profiling_context.cpp
            core/search_space.cpp
            model/agree_set_sample.cpp
            model/confidence_interval.cpp
            model/pli_cache.cpp
            model/list_agree_set_sample.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE spdlog::spdlog_header_only ${DESBORDANTE_PREFIX}::model::table Boost::headers
)
