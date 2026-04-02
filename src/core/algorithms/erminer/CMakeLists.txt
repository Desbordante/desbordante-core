# Sequence
set(NAME erminer.sequence)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Sequence.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/Sequence.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

# SequenceDatabase
set(NAME erminer.sequencedatabase)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/SequenceDatabase.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/SequenceDatabase.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PUBLIC ${DESBORDANTE_PREFIX}::erminer::sequence)

# Occurence
set(NAME erminer.occurence)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Occurence.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/Occurence.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

# LeftRule
set(NAME erminer.leftrule)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/LeftRule.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/LeftRule.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

# RightRule
set(NAME erminer.rightrule)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/RightRule.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/RightRule.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

# LeftEquivalenceClass
set(NAME erminer.leftequivalenceclass)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/LeftEquivalenceClass.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/LeftEquivalenceClass.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PUBLIC ${DESBORDANTE_PREFIX}::erminer::occurence ${DESBORDANTE_PREFIX}::erminer::leftrule)

# RightEquivalenceClass
set(NAME erminer.rightequivalenceclass)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/RightEquivalenceClass.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/RightEquivalenceClass.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PUBLIC ${DESBORDANTE_PREFIX}::erminer::occurence ${DESBORDANTE_PREFIX}::erminer::rightrule)

# ExpandLeftStore
set(NAME erminer.expandleftstore)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ExpandLeftStore.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/ExpandLeftStore.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PUBLIC ${DESBORDANTE_PREFIX}::erminer::occurence ${DESBORDANTE_PREFIX}::erminer::leftrule ${DESBORDANTE_PREFIX}::erminer::leftequivalenceclass)

# SparseMatrix
set(NAME erminer.sparsematrix)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/SparseMatrix.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/SparseMatrix.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

# Itemset
set(NAME erminer.itemset)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Itemset.cpp 
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/Itemset.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)

# Основной алгоритм
set(NAME erminer)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ERMiner.cpp
    PUBLIC FILE_SET HEADERS 
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/ERMiner.hpp 
        BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::algos ${DESBORDANTE_PREFIX}::config Boost::headers
    PRIVATE ${DESBORDANTE_PREFIX}::erminer::sequencedatabase 
            ${DESBORDANTE_PREFIX}::erminer::occurence
            ${DESBORDANTE_PREFIX}::erminer::leftrule
            ${DESBORDANTE_PREFIX}::erminer::rightrule
            ${DESBORDANTE_PREFIX}::erminer::leftequivalenceclass
            ${DESBORDANTE_PREFIX}::erminer::rightequivalenceclass
            ${DESBORDANTE_PREFIX}::erminer::expandleftstore
            ${DESBORDANTE_PREFIX}::erminer::sparsematrix
            ${DESBORDANTE_PREFIX}::erminer::itemset
            spdlog::spdlog_header_only
)