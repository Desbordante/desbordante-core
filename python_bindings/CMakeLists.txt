SET(BINDINGS_NAME desbordante)

file(GLOB_RECURSE sources "*.h*" "*.cpp*")
pybind11_add_module(${BINDINGS_NAME} ${sources})

target_link_libraries(${BINDINGS_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_lib easyloggingpp)
