set(NAME cfd.cfdfinder.util)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE util/lhs_utils.cpp util/pli_cache.cpp util/support_map.cpp)
target_link_libraries(${NAME} PRIVATE Boost::headers)

set(NAME cfd.cfdfinder.model)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE model/expansion/constant_expansion_strategy.cpp
            model/expansion/expansion_strategy.cpp
            model/expansion/positive_negative_constant_strategy.cpp
            model/expansion/range_pattern_expansion.cpp
            model/hyfd/inductor.cpp
            model/hyfd/preprocessor.cpp
            model/hyfd/validator.cpp
            model/pattern/pattern.cpp
            model/pattern/pattern_tableau.cpp
            model/pruning/legacy_pruning_strategy.cpp
            model/pruning/partial_fd_pruning.cpp
            model/pruning/support_independent_strategy.cpp
            model/cfdfinder_relation_data.cpp
)
target_link_libraries(
    ${NAME}
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::cfdfinder::util ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::fd::hy Boost::headers ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only
)

set(NAME cfd.cfdfinder.types)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE types/frontier.cpp types/columns.cpp)

set(NAME cfd.cfdfinder)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE cfdfinder.cpp cfd.cpp)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::cfd::cfdfinder::util
            ${DESBORDANTE_PREFIX}::cfd::cfdfinder::model
            ${DESBORDANTE_PREFIX}::cfd::cfdfinder::types
            ${DESBORDANTE_PREFIX}::algos
            ${DESBORDANTE_PREFIX}::util
            spdlog::spdlog_header_only
            magic_enum::magic_enum
            Boost::headers
)
