set(NAME datastats)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE data_stats.cpp statistic.cpp)
if(APPLE)
    execute_process(
        COMMAND brew --prefix icu4c
        OUTPUT_VARIABLE ICU_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ICU_ROOT ${ICU_ROOT})
endif()
find_package(ICU REQUIRED COMPONENTS uc i18n)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::model::table
            ${DESBORDANTE_PREFIX}::model::types
            ${DESBORDANTE_PREFIX}::algos
            magic_enum::magic_enum
            Boost::headers
            ICU::uc
            ICU::i18n
)
