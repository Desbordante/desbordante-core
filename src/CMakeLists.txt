set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE lib_sources "*/*.h*" "*/*.cpp*" "*/*.cc*")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(${BINARY}_lib STATIC ${lib_sources})
set_property(TARGET ${BINARY}_lib PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${BINARY}_lib PRIVATE ${Boost_LIBRARIES} Threads::Threads)
target_link_libraries(${BINARY}_lib PUBLIC easyloggingpp)

option(SAFE_VERTICAL_HASHING
        "Enable safe vertical hashing. This feature allows to process\
         wide (>32 columns) datasets, possibly lowering the performance."
        OFF)
if (SAFE_VERTICAL_HASHING)
    target_compile_definitions(${BINARY}_lib PUBLIC SAFE_VERTICAL_HASHING)
endif(SAFE_VERTICAL_HASHING)

set(run_sources "main.cpp")

add_executable(${BINARY}_run ${run_sources})
target_link_libraries(${BINARY}_run PRIVATE ${CMAKE_PROJECT_NAME}_lib)
