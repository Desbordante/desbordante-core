find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM_DIR: ${LLVM_DIR}")

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
target_include_directories(${NAME} PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(checks)