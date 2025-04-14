# ---- Python Bindings Configuration ----

# Define the output name for Python bindings module
# NOTE: Matches the Python import name (e.g. 'import desbordante')
set(BINDINGS_NAME desbordante)

# Auto-discover all source files for bindings
file(GLOB_RECURSE sources "*.h*" "*.cpp*")

# Create Python-native module using pybind11
# Generates SHARED library with Python-compatible ABI
pybind11_add_module(${BINDINGS_NAME} ${sources})

# Configure output directory to central build folder
# Ensures generated .so/.pyd files are placed alongside other binaries
set_target_properties(
    ${BINDINGS_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} # ${CMAKE_BINARY_DIR}/target
)

# Link against core library containing algorithm implementations
# PRIVATE visibility: Python module depends on but doesn't expose core internals
target_link_libraries(
    ${BINDINGS_NAME} PRIVATE ${CMAKE_PROJECT_NAME} # Desbordante core static library
)

# Enable local header inclusion
include_directories(".")

# ---- Installation Setup ----

# Optional installation of Python module during `make install`
# Activated when PYTHON=INSTALL via command line (-DPYTHON=INSTALL)
if(PYTHON STREQUAL "INSTALL")
    # Install shared library to Python's site-packages
    # LIBRARY DESTINATION: Respects platform-specific Python lib locations
    install(TARGETS ${BINDINGS_NAME} LIBRARY DESTINATION .)
endif()
