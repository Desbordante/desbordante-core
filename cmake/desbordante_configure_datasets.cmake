include_guard(DIRECTORY)

set(BASE_URL "https://raw.githubusercontent.com/Desbordante/desbordante-data/refs/heads/main")

#[=[
    Brief
    ----
        Download a dataset from the desbordante-data repo with hash verification.
        The function downloads a zip file (if changed) and its corresponding SHA256 hash file
        during the configuration.
    Params
    ----
        NAME (keyword): str
            The base name of the dataset (e.g., `dataset`). The function expects
            ``dataset.zip`` and ``dataset.sha256`` to exist at the remote source.
        DOWNLOAD_DIR (keyword): str
            Local directory where the zip and hash files will be stored. The directory
            must exist. If a relative directory is given, it is interpreted as being
            relative to the current binary directory.
    Usage
    ----
        desbordante_fetch_datasets(<stamp_var>
        [NAME <filename>] [DOWNLOAD_DIR <dir>])
        ```
        desbordante_fetch_datasets(
            NAME data
            DOWNLOAD_DIR ${DOWNLOAD_DIR}
        )
        ```
]=]
function(desbordante_fetch_datasets)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "NAME;DOWNLOAD_DIR" "")

    set(filename "${arg_NAME}.zip")
    set(file_path "${arg_DOWNLOAD_DIR}/${filename}")

    set(hashfile_name "${arg_NAME}.sha256")
    set(local_hashfile "${arg_DOWNLOAD_DIR}/${hashfile_name}")
    set(hashfile_url "${BASE_URL}/${hashfile_name}")

    file(DOWNLOAD ${hashfile_url} ${local_hashfile} STATUS download_status)
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        list(GET download_status 1 str_val)
        message(
            WARNING
                "Failed to download ${hashfile_url}: [${status_code}] ${str_val}. Fetch skipped \
                for ${filename}. If you already have the necessary datasets and do not want to \
                check for updates, consider setting -DDESBORDANTE_FETCH_DATASETS=OFF."
        )
        # local hashfiles are empty
        file(REMOVE ${local_hashfile})
        return()
    endif()

    file(READ "${local_hashfile}" remote_hash)
    string(STRIP "${remote_hash}" remote_hash)

    message(STATUS "-> Fetching ${filename}")
    file(
        DOWNLOAD "${BASE_URL}/${filename}" "${file_path}"
        EXPECTED_HASH SHA256=${remote_hash}
        STATUS download_status
    )

    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        list(GET download_status 1 str_val)
        message(FATAL_ERROR "Failed to download ${filename}: [${status_code}] ${str_val}.")
    endif()
endfunction()

#[=[
    Brief
    ----
        Unzip a dataset.
        The extraction is defined via `add_custom_command` and occurs
        during the build phase. A stamp file is used to ensure extraction happens only when the
        source zip changes.
        Note, this function defines the build rules but does not create a target.
        You must manually hook the output stamp file into your build graph
        (e.g., via `add_custom_target` and `add_dependencies()`), see Usage below.
    Params
    ----
        stamp_file_var (output): var
            The name of the variable (in parent scope) that will store the path to the
            generated extraction stamp file.
        PATH (keyword): str
            The path to the dataset with .zip extension (e.g., `dir/dataset.zip`). The function
            expects ``dataset.zip`` to exist at the local. If a relative directory is
           given, it is interpreted as being relative to the current binary directory, but prefer
           absolute paths.
        EXTRACT_DIR (keyword): str
           Local directory where the zip contents will be extracted. If a relative directory is
           given, it is interpreted as being relative to the current binary directory, but prefer
           absolute paths.
    Return
    ----
        Propagates a variable to the parent scope containing the path:
        "<EXTRACT_DIR>/<NAME>.extracted".
    Usage
    ----
        desbordante_unpack_datasets(<stamp_var>
        [PATH <path_to_file>] [EXTRACT_DIR <dir>]))
        ```
        desbordante_unpack_datasets(
            MY_DATA_STAMP
            PATH ${CMAKE_CURRENT_LIST_DIR}/data.zip
            EXTRACT_DIR ${EXTRACT_DIR}
        )
        # Here, a var named `MY_DATA_STAMP` now equals to
        # `"${EXTRACT_DIRECTORY}/test_dataset.extracted"`
        add_custom_target(test_data DEPENDS ${STAMP_VAR})
        add_dependencies(test_bin test_data)
        ```
]=]
function(desbordante_unpack_datasets stamp_file_var)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "PATH;EXTRACT_DIR" "")
    set(filename $<PATH:GET_FILENAME,${arg_PATH}>)
    set(${stamp_file_var} "${arg_EXTRACT_DIR}/${filename}.extracted")
    add_custom_command(
        OUTPUT ${${stamp_file_var}}
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${arg_PATH}
        COMMAND ${CMAKE_COMMAND} -E touch ${${stamp_file_var}}
        WORKING_DIRECTORY ${arg_EXTRACT_DIR}
        DEPENDS ${arg_PATH}
        COMMENT "Extracting ${filename}"
    )
    return(PROPAGATE ${stamp_file_var})
endfunction()
