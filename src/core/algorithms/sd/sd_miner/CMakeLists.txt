set(NAME sd.miner)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE sd_miner.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::sd::util ${DESBORDANTE_PREFIX}::model::table
                    Boost::headers magic_enum::magic_enum
)
