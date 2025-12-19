include_guard(DIRECTORY)

#[=[
    Brief
    ----
        Download and unzip a dataset zip file from the desbordante-data repository, using a stamp
        file to track extraction completion.
        The downloading occurs during configuration time, while extracting in build. This allows
        extracting to occur only once. To unzip again, you must delete the stamp file.
        Note that you must provide add_custom_target() and add_dependencies() by yourself (see Usage
        below)
    Params
    ----
        stamp_file (output): var
            Variable name that will hold the path to the extraction stamp file.
        NAME (keyword): str
            Base name of the dataset (without .zip extension).
        SHA2 (keyword): str
            SHA256 hash of the file for download verification.
        DOWNLOAD_DIR (keyword): str
            Directory where the zip file will be downloaded. If a relative directory is given, it is
            interpreted as being relative to the current binary directory.
        EXTRACT_DIR (keyword): str
            Directory where contents will be extracted. If a relative directory is given, it is
            interpreted as being relative to the current binary directory.
    Return
    ----
        Propagates the stamp_file variable to the parent scope, containing the path to the
        extraction stamp file (format: "<EXTRACT_DIR>/<NAME>.<SHA2>.extracted").
    Usage
    ----
        desbordante_configure_datasets(<stamp_var>
        [NAME <file_name>] [SHA2 <sha256sum>] [DOWNLOAD_DIR <d_dir>] [EXTRACT_DIR <e_dir>])
        ```
        desbordante_configure_datasets(
            STAMP_FILE
            NAME test_dataset
            SHA2 12345
            DOWNLOAD_DIR ${DOWNLOAD_DIRECTORY}
            EXTRACT_DIR ${EXTRACT_DIRECTORY}
        )
        # Here, a var named `STAMP_VAR` now equals to
        # `"${EXTRACT_DIRECTORY}/test_dataset.12345.extracted"`
        add_custom_target(test_data DEPENDS ${STAMP_VAR})
        add_dependencies(test_bin test_data)
        ```
]=]
function(desbordante_configure_datasets stamp_file)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "NAME;SHA2;DOWNLOAD_DIR;EXTRACT_DIR" "")
    set(file_path "${arg_DOWNLOAD_DIR}/${arg_NAME}.zip")
    file(
        DOWNLOAD
        https://raw.githubusercontent.com/Desbordante/desbordante-data/refs/heads/main/${arg_NAME}.zip
        ${file_path}
        EXPECTED_HASH SHA256=${arg_SHA2}
    )

    set(${stamp_file} "${arg_EXTRACT_DIR}/${arg_NAME}.${arg_SHA2}.extracted")
    add_custom_command(
        OUTPUT ${${stamp_file}}
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${file_path}
        COMMAND ${CMAKE_COMMAND} -E touch ${${stamp_file}}
        WORKING_DIRECTORY ${arg_EXTRACT_DIR}
        DEPENDS ${file_path}
        COMMENT "Extracting ${arg_NAME}.zip"
    )
    return(PROPAGATE ${stamp_file})
endfunction()
