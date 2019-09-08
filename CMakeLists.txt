# Only for code highlight in CLion

cmake_minimum_required(VERSION 3.10)
project(metanome_cpp)
set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "../Software/boost_1_70_0/boost_output")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "../Software/boost_1_70_0/boost_output/lib")
find_package(Boost 1.70.0 COMPONENTS filesystem regex container)

#if(Boost_FOUND)
#include_directories(container)
    #add_executable(progname file1.cxx file2.cxx)
    #target_link_libraries(metanome_cpp ${Boost_LIBRARIES})
#ADD_DEFINITIONS( "-DHAS_BOOST" )
#endif()

file(GLOB_RECURSE metanome_cpp_src "src/*.h*" "src/*.cpp*" )

add_executable(metanome_cpp ${metanome_cpp_src})
target_link_libraries(metanome_cpp PRIVATE "stdc++fs" Boost::container)