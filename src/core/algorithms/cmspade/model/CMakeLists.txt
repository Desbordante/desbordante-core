set(NAME cmspade.model)
desbordante_add_lib(NAME OBJECT)

target_sources(
    ${NAME}
    PRIVATE
        id_list.cpp
        itemset.cpp
        pattern.cpp
        sequence.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(
    ${NAME}
    PUBLIC Boost::headers
)