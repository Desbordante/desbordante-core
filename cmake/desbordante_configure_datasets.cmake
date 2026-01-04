include_guard(DIRECTORY)

set(BASE_URL "https://raw.githubusercontent.com/Desbordante/desbordante-data/refs/heads/main")

#[=[
    Brief
    ----
        Download and unzip a dataset from the desbordante-data repo with hash verification and
        build-time extraction.
        The function downloads a zip file (if changed) and its corresponding SHA256 hash file
        during the configuration. The extraction is defined via `add_custom_command` and occurs
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
        NAME (keyword): str
            The base name of the dataset (e.g., `dataset`). The function expects
            ``dataset.zip`` and ``dataset.sha256`` to exist at the remote source.
        DOWNLOAD_DIR (keyword): str
            Local directory where the zip and hash files will be stored. The directory
            must exist. If a relative directory is given, it is interpreted as being
            relative to the current binary directory.
        EXTRACT_DIR (keyword): str
           Local directory where the zip contents will be extracted. If a relative directory is
           given, it is interpreted as being relative to the current binary directory.
    Return
    ----
        Propagates a variable to the parent scope containing the path:
        "<EXTRACT_DIR>/<NAME>.extracted".
    Usage
    ----
        desbordante_configure_datasets(<stamp_var>
        [NAME <filename>] [DOWNLOAD_DIR <d_dir>] [EXTRACT_DIR <e_dir>])
        ```
        desbordante_configure_datasets(
            MY_DATA_STAMP
            NAME test_dataset
            DOWNLOAD_DIR ${DOWNLOAD_DIR}
            EXTRACT_DIR ${EXTRACT_DIR}
        )
        # Here, a var named `MY_DATA_STAMP` now equals to
        # `"${EXTRACT_DIRECTORY}/test_dataset.extracted"`
        add_custom_target(test_data DEPENDS ${STAMP_VAR})
        add_dependencies(test_bin test_data)
        ```
]=]
function(desbordante_configure_datasets stamp_file_var)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "NAME;DOWNLOAD_DIR;EXTRACT_DIR" "")

    set(filename "${arg_NAME}.zip")
    set(file_path "${arg_DOWNLOAD_DIR}/${filename}")

    if(NOT DESBORDANTE_FETCH_DATASETS)
        message(WARNING "There could be updates in dataset ${filename} (Fetch disabled)")
    else()
        set(hashfile_name "${arg_NAME}.sha256")
        set(local_hashfile "${arg_DOWNLOAD_DIR}/${hashfile_name}")

        file(DOWNLOAD "${BASE_URL}/${hashfile_name}" STATUS download_status)
        list(GET download_status 0 status_code)
        if(NOT status_code EQUAL 0)
            list(GET download_status 1 str_val)
            set(message_mode WARNING)
            if(NOT EXISTS ${local_hashfile})
                set(message_mode "FATAL_ERROR")
            endif()
            message(
                    "${message_mode}" "${hashfile_name} cannot be downloaded: [${status_code}] ${str_val}."
            )
            return()
        endif()

        file(DOWNLOAD "${BASE_URL}/${hashfile_name}" "${local_hashfile}" STATUS download_status)
        list(GET download_status 0 status_code)
        if(NOT status_code EQUAL 0)
            list(GET download_status 1 str_val)
            message(
                    WARNING "Failed to download hash for ${filename}: [${status_code}] ${str_val}. Skip"
            )
            return()
        endif()

        file(READ "${local_hashfile}" remote_hash)
        string(STRIP "${remote_hash}" remote_hash)

        set(need_download ON)
        if(EXISTS "${file_path}")
            file(SHA256 "${file_path}" local_hash)
            if("${local_hash}" STREQUAL "${remote_hash}")
                message(STATUS "-> ${filename} is up to date.")
                set(need_download OFF)
            else()
                message(STATUS "-> Hashes differ (Remote: ${remote_hash} vs Local: ${local_hash})")
            endif()
        endif()

        if(need_download)
            message(STATUS "-> Download ${filename}")
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
        endif()
    endif()

    set(${stamp_file_var} "${arg_EXTRACT_DIR}/${arg_NAME}.extracted")
    add_custom_command(
        OUTPUT ${${stamp_file_var}}
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${file_path}
        COMMAND ${CMAKE_COMMAND} -E touch ${${stamp_file_var}}
        WORKING_DIRECTORY ${arg_EXTRACT_DIR}
        DEPENDS ${file_path}
        COMMENT "Extract ${filename}"
    )
    return(PROPAGATE ${stamp_file_var})
endfunction()
