set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE lib_sources "*/*.h*" "*/*.cpp*" "*/*.cc*")

set(run_sources "main.cpp")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_library(PQXX_LIB pqxx)
if (DEFINED ENV{LIBRDKAFKA_ROOT})
    set(LIBRDKAFKA_ROOT $ENV{LIBRDKAFKA_ROOT})
else ()
    set(LIBRDKAFKA_ROOT /usr/local)
endif ()

add_executable(${BINARY}_kafka_server ${run_sources})

target_link_libraries(${BINARY}_kafka_server PUBLIC ${PQXX_LIB} ${CMAKE_PROJECT_NAME}_lib rdkafka pthread)
target_link_libraries(${BINARY}_kafka_server LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads)
