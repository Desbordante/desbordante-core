include_guard(GLOBAL)

set(FLAGS)

if(DESBORDANTE_BUILD_NATIVE)
   list(APPEND FLAGS -march=native)
endif()

if(DESBORDANTE_GDB_SYMBOLS)
    list(APPEND FLAGS -ggdb3)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND FLAGS -Wall -Wextra -fno-omit-frame-pointer -fno-optimize-sibling-calls)
endif()

if(DESBORDANTE_SANITIZER)
    list(APPEND FLAGS
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=float-divide-by-zero
            -Wno-error # Use of -Werror is discouraged with sanitizers
            # See https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003dbuiltin
            -fno-sanitize=signed-integer-overflow # Remove this when CustomRandom gets fixed
            -fno-sanitize=shift # Remove this when CustomRandom gets fixed
            -fno-sanitize-recover=all)
    if(NOT CMAKE_BUILD_TYPE MATCHES Release)
        list(APPEND FLAGS "-O1")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    # Workaround for Clang bug in versions 18 and above:
    # See issue https://github.com/llvm/llvm-project/issues/76515
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "18")
        message(WARNING "C++ compiler is Clang++-${CMAKE_CXX_COMPILER_VERSION}. "
                "Suppressing deprecated declaration warnings. "
                "Consider using another version of Clang."
        )
        list(APPEND FLAGS -Wno-deprecated-declarations)
    endif()

    if(CMAKE_HOST_APPLE)
        # Limit some UB sanitizer checks to "src" directory on macOS when building with Clang,
        # because libraries (STL, googletest, boost, etc.) are somehow broken
        list(APPEND FLAGS -fsanitize-ignorelist=${CMAKE_SOURCE_DIR}/ub_sanitizer_ignore_list.txt)
    endif()
endif()

if(FLAGS)
    add_compile_options(${FLAGS})
endif()