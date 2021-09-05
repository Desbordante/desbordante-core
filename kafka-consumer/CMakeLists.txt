set(BINARY ${CMAKE_PROJECT_NAME})

# file(GLOB_RECURSE lib_sources "*/*.h*" "*/*.cpp*" "*/*.cc*")

# add_library(${BINARY}_lib STATIC ${lib_sources})

set(run_sources "main.cpp")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_library(PQXX_LIB pqxx)
find_package(CppKafka REQUIRED)

add_executable(${BINARY}_kafka_consumer ${run_sources})

target_link_libraries(${BINARY}_kafka_consumer PUBLIC ${CMAKE_PROJECT_NAME}_lib ${PQXX_LIB} Boost::thread rdkafka CppKafka::cppkafka pthread)
target_link_libraries(${BINARY}_kafka_consumer LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads)
