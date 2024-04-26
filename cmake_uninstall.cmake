if(NOT EXISTS "@CMAKE_CURRENT_BINARY_DIR@/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: @CMAKE_CURRENT_BINARY_DIR@/install_manifest.txt")
endif()

file(READ "@CMAKE_CURRENT_BINARY_DIR@/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
    message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
    if(EXISTS "$ENV{DESTDIR}${file}")
        execute_process(
            COMMAND @CMAKE_COMMAND@ -E remove "$ENV{DESTDIR}${file}"
            RESULT_VARIABLE rm_exit_code
            OUTPUT_VARIABLE rm_out
            ERROR_VARIABLE rm_err
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT "${rm_err}" STREQUAL "")
            message(STATUS "Error when removing \"$ENV{DESTDIR}${file}\": ${rm_err}")
        endif()
    else()
        message(STATUS "File \"$ENV{DESTDIR}${file}\" does not exist.")
    endif()
endforeach(file)
