set(NAME ind.spider)
desbordante_add_lib(NAME)
target_sources(${NAME} PRIVATE spider.cpp attribute.cpp)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::util ${DESBORDANTE_PREFIX}::model::table Boost::headers
)
