include_guard(GLOBAL)

set(FLAGS)

if(DESBORDANTE_BUILD_NATIVE)
   list(APPEND FLAGS "-march=native")
endif()

if(DESBORDANTE_GDB_SYMBOLS)
    list(APPEND FLAGS "-ggdb3")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND FLAGS "-Wall -Wextra -fno-omit-frame-pointer -fno-optimize-sibling-calls")
endif()

if(DESBORDANTE_SANITIZER)
    list(APPEND FLAGS
            "-fsanitize=address"
            "-fsanitize=undefined"
            "-fsanitize=float-divide-by-zero"
            "-Wno-error" # Use of -Werror is discouraged with sanitizers
            # See https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003dbuiltin
            "-fno-sanitize=signed-integer-overflow" # Remove this when CustomRandom gets fixed
            "-fno-sanitize=shift" # Remove this when CustomRandom gets fixed
            "-fno-sanitize-recover=all")
    if(NOT CMAKE_BUILD_TYPE MATCHES Release)
        list(APPEND FLAGS "-O1")
    endif()
endif()

if(FLAGS)
    string(REPLACE ";" " " FLAGS "${FLAGS}")
    set(CMAKE_CXX_FLAGS_INIT "${FLAGS}")
endif()