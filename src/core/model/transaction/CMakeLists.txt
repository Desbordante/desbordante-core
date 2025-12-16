set(NAME model.transaction)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE itemset.cpp transactional_data.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
