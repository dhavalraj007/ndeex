function(add_shaders TARGET_NAME)
  set(SHADER_SOURCE_FILES ${ARGN})

  set(SPV_FILES)

  foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
    cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
    cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)

    set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
      OUTPUT ${OUTPUT_FILE}
      COMMAND Vulkan::glslc "${SHADER_SOURCE}" --target-env=vulkan -o "${OUTPUT_FILE}"
      DEPENDS ${SHADER_SOURCE}
      COMMENT "Compiling shader ${SHADER_SOURCE} -> ${OUTPUT_FILE}"
    )

    list(APPEND SPV_FILES ${OUTPUT_FILE})
  endforeach()

  add_custom_target(${TARGET_NAME} ALL
    DEPENDS ${SPV_FILES}
  )
endfunction()
