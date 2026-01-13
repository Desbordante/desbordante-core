include_guard(GLOBAL)

set(DESBORDANTE_PREFIX
    "Desbordante"
    CACHE STRING ""
)

set(NAME "${DESBORDANTE_PREFIX}.compile_feats")
add_library(${NAME} INTERFACE)
target_compile_features(${NAME} INTERFACE cxx_std_20)
target_compile_definitions(
    ${NAME} INTERFACE $<$<BOOL:${DESBORDANTE_SAFE_VERTICAL_HASHING}>:DESBORDANTE_SAFE_VERTICAL_HASHING>
)

#[=[
    Brief
    ----
        Add a library to the project with aliases, properties and compile features required for all
        libraries in Desbordante.
        Note that although it's possible to list sources, i.e. desbordante_add_lib(<name> file.cpp),
        it is preferable to use `target_sources`.
    Params
        name : var, str A variable that has been set to the desired name OR a string equal to the
        desired name other parameters as in add_library
    Return
    ----
        Propagates name constructed with a prefix.
    Usage
    ----
        desbordante_add_lib(<name> [<type>] [EXCLUDE_FROM_ALL] <sources>…)
        ```
        # 1) if name is a variable
        set(NAME A.B)
        desbordante_add_lib(NAME)
        # Here, a variable named `NAME` now equals to `${DESBORDANTE_PREFIX}.A.B`. Also there is
        # alias `${DESBORDANTE_PREFIX}::A::B`
        target_sources(${NAME} …)

        # 2) if name is a string
        desbordante_add_lib(A.B)
        # Here, `${DESBORDANTE_PREFIX}.A.B` and its alias`${DESBORDANTE_PREFIX}::A::B` exist
        target_sources("${DESBORDANTE_PREFIX}.A.B" …)
        ```
]=]
function(desbordante_add_lib name)
    set(is_name_string OFF)
    set(original_name ${${name}})
    if(NOT DEFINED original_name)
        set(is_name_string ON)
        set(original_name ${name})
    endif()
    set(full_name "${DESBORDANTE_PREFIX}.${original_name}")

    add_library(${full_name} ${ARGN})

    set_target_properties(${full_name} PROPERTIES LINK_LIBRARIES_ONLY_TARGETS ON)

    if(ARGV1 STREQUAL "INTERFACE")
        target_link_libraries(${full_name} INTERFACE ${DESBORDANTE_PREFIX}.compile_feats)
    else()
        target_link_libraries(${full_name} PUBLIC ${DESBORDANTE_PREFIX}.compile_feats)
    endif()

    string(REPLACE "." "::" alias ${full_name})
    add_library(${alias} ALIAS ${full_name})

    if(NOT is_name_string)
        set(${name} ${full_name})
        return(PROPAGATE ${name})
    endif()
endfunction()

#[=[
    Brief
    ----
        Add a binding object library called "${DESBORDANTE_PREFIX}.bind.<name>" to the project with
        aliases, properties, compile features, libraries required for all bindings in Desbordante.
    Params
    ----
        name : str  A name of the desired bind
        SRCS : list Sources to use when building a binding
        LIBS : list Libraries or flags to use when linking a binding
    Usage
    ----
        desbordante_add_bind(<name> [SRCS <sources>…] [LIBS <items>…])
        ```
        desbordante_add_bind(algo SRCS algo.cpp LIBS lib::algo)
        # Here, an Object Library ${DESBORDANTE_PREFIX}.bind.algo exists
        ```
]=]
function(desbordante_add_bind name)
    cmake_parse_arguments(arg "" "" "SRCS;LIBS" ${ARGN})

    set(bind_name "bind.${name}")
    desbordante_add_lib(bind_name OBJECT)

    target_sources(
        ${bind_name}
        PRIVATE ${arg_SRCS}
        PRIVATE FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    )

    list(APPEND arg_LIBS pybind11::pybind11 ${DESBORDANTE_PREFIX}.compile_feats)
    target_link_libraries(${bind_name} PRIVATE ${arg_LIBS})

    target_link_libraries(${MODULE_NAME} PRIVATE ${bind_name})
endfunction()

#[=[
    Brief
    ----
        Add a test called "${DESBORDANTE_PREFIX}.test.<name>" to the project with properties,
        compile features, libraries required for all tests in Desbordante.
    Params
    ----
        name : str  A name of the desired test
        SRCS : list Sources to use when building a test
        LIBS : list Libraries or flags to use when linking a test
    Usage
    ----
        desbordante_add_test(<name> [SRCS <sources>…] [LIBS <items>…])
        ```
        desbordante_add_test(algo SRCS test_algo.cpp LIBS lib::algo)
        # Here, a test ${DESBORDANTE_PREFIX}.test.algo exists
        ```
]=]
function(desbordante_add_test name)
    cmake_parse_arguments(arg "" "" "SRCS;LIBS" ${ARGN})

    set(name "${DESBORDANTE_PREFIX}.test.${name}")
    add_executable(${name})
    set_target_properties(${name} PROPERTIES LINK_LIBRARIES_ONLY_TARGETS ON)

    target_sources(${name} PRIVATE ${arg_SRCS})

    list(APPEND arg_LIBS ${DESBORDANTE_PREFIX}.testlib.main gtest
         ${DESBORDANTE_PREFIX}.compile_feats
    )
    target_link_libraries(${name} PRIVATE ${arg_LIBS})

    gtest_discover_tests(${name} WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endfunction()
