SET(BINDINGS_NAME desbordante)

file(GLOB_RECURSE sources "*.h*" "*.cpp*")
pybind11_add_module(${BINDINGS_NAME} ${sources})

# TODO(polyntsov): Explicitly link with the shared library, static won't work.
# This solution seems ugly since I don't fully understand which part of the python bindings library
# uses Boost::thread, but without it `import desbordante` will crash with undefined symbol error.
# We need to investigate the problem and remove the dependency on Boost::thread
find_library(BOOST_THREAD libboost_thread.so libboost_thread)
target_link_libraries(${BINDINGS_NAME} PRIVATE ${CMAKE_PROJECT_NAME}_lib easyloggingpp ${BOOST_THREAD})
