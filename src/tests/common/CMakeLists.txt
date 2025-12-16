set(NAME testlib.common)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE all_csv_configs.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
