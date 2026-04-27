set(NAME datastats)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE data_stats.cpp statistic.cpp)
find_package(ICU REQUIRED COMPONENTS uc i18n)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table ${DESBORDANTE_PREFIX}::model::types
                    ${DESBORDANTE_PREFIX}::algos better-enums Boost::headers
		    ICU::uc ICU::i18n
)
