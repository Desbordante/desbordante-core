# --- AFDMetricCalc ---
set(NAME afd.metric_calc)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE afd_metric_calculator.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::util ${DESBORDANTE_PREFIX}::model::table 
        ${DESBORDANTE_PREFIX}::config Boost::headers better-enums
)

