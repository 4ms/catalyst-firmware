function(preprocess_link_script target_base link_script_template)
  get_filename_component(LINK_SCRIPT_FILENAME ${link_script_template} NAME_WE)

  set(LINK_SCRIPT_PROCESSED ${CMAKE_CURRENT_BINARY_DIR}/${LINK_SCRIPT_FILENAME}-cpp.ld)

  add_custom_command(
    OUTPUT ${LINK_SCRIPT_PROCESSED}
    DEPENDS ${link_script_template}
    COMMAND bash ARGS -c "gcc -E -x c -CC -I. ${link_script_template} | grep -v '^#' > ${LINK_SCRIPT_PROCESSED}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
  )
  set_target_properties(${target_base}.elf PROPERTIES LINK_DEPENDS ${LINK_SCRIPT_PROCESSED})

  add_custom_target(${target_base}-preproc-linkscript DEPENDS ${LINK_SCRIPT_PROCESSED})
  add_dependencies(${target_base}.elf ${target_base}-preproc-linkscript)

  target_link_options(
    ${target_base}.elf 
    PRIVATE
        -Wl,-Map,$<TARGET_FILE_DIR:${target_base}.elf>/${target_base}.map,--cref 
        -T ${LINK_SCRIPT_PROCESSED} 
        ${ARGN}
  )
endfunction()

function(target_link_script target_base link_script)
  target_link_options(
    ${target_base}.elf 
    PRIVATE
        -Wl,-Map,$<TARGET_FILE_DIR:${target_base}.elf>/${target_base}.map,--cref 
        -T ${link_script} 
        ${ARGN}
  )
  set_target_properties(${target_base}.elf PROPERTIES LINK_DEPENDS ${link_script})
endfunction()

function(add_bin_hex_command target_base)
  set(BASENAME $<TARGET_FILE_DIR:${target_base}.elf>/${target_base})
  add_custom_command(
    TARGET ${target_base}.elf
    POST_BUILD
    COMMAND echo "Built .elf file: $<TARGET_FILE:${target_base}.elf>"
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${target_base}.elf> ${BASENAME}.hex
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${target_base}.elf> ${BASENAME}.bin
  )
  set_target_properties(
    ${target_base}.elf
    PROPERTIES ADDITIONAL_CLEAN_FILES "${BASE_NAME}.hex;${BASE_NAME}.bin;${BASE_NAME}.map"
  )
endfunction()
