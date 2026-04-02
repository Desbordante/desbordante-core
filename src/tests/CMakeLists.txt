add_subdirectory(common)

if(DESBORDANTE_BUILD_TESTS)
    add_subdirectory(unit)
endif()
if(DESBORDANTE_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
