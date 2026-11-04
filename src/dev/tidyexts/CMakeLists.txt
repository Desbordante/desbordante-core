find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR: ${LLVM_DIR}")

if(LLVM_PACKAGE_VERSION VERSION_LESS "21.0")
    message(WARNING "Tidyexts has not been confirmed to work on versions of LLVM less than 21.0")
endif()

set(NAME tidyexts)
desbordante_add_lib(NAME SHARED)
target_sources(${NAME} PRIVATE module.cpp)
target_link_libraries(${NAME} PRIVATE
    clangTidy
    clangAST
    clangASTMatchers
    clangBasic
    clangTooling
)
target_include_directories(${NAME} SYSTEM PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(checks)