execute_process(
    COMMAND "${EXECUTABLE}"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(NOT result STREQUAL EXPECTED_EXIT)
    message(FATAL_ERROR
        "Expected exit code ${EXPECTED_EXIT}, got ${result}.\n"
        "stdout: ${output}\n"
        "stderr: ${error}")
endif()
