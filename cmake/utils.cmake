# Output Directories
set(OUTPUT_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BIN_DIR}")

foreach (CONFIG ${CONFIG_TYPES})
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BIN_DIR}")
endforeach ()

function(set_output_subdir SUBDIR)
    foreach (TGT ${ARGN})
        set_target_properties(${TGT} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BIN_DIR}/${SUBDIR}")
        foreach (CONFIG ${CONFIG_TYPES})
            string(TOUPPER "${CONFIG}" CONFIG_UPPER)
            set_target_properties(${TGT} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BIN_DIR}/${SUBDIR}")
        endforeach ()
    endforeach ()
endfunction()
