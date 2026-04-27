set(NAME tks)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE tks.cpp Bitmap.cpp Candidate.cpp Itemset.cpp TKSEngine.cpp
)
target_link_libraries(
    ${NAME} PRIVATE ${DESBORDANTE_PREFIX}::model::table 
            ${DESBORDANTE_PREFIX}::algos
            ${DESBORDANTE_PREFIX}::parser::spmf
            better-enums Boost::headers
)
