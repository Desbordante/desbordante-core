cmake_minimum_required(VERSION 3.10)
project(metanome_cpp)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE metanome_cpp_src "src/*.h*" "src/*.cpp*")

add_executable(metanome_cpp ${metanome_cpp_src})
#target_link_libraries(metanome_cpp PRIVATE "stdc++fs")