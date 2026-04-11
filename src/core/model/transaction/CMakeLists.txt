set(NAME model.transaction)
desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE itemset.cpp transactional_data.cpp)
target_link_libraries(${NAME} PRIVATE better-enums)

