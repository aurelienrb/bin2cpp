add_executable(bin2cpp src/main.cpp)
target_link_libraries(bin2cpp stdc++fs)


add_custom_command(
    OUTPUT ${OUTPUT_FILES}
    COMMAND bin2cpp
    ARGS -ns WebUI -d ${OUTPUT_DIR} -o ${OUTPUT_BASE_NAME} ${INPUT_FILES}
    DEPENDS ${INPUT_FILES}
    COMMENT "Running bin2cpp on webui html files"
    VERBATIM
)

add_library(router.webui.html_files ${OUTPUT_FILES})
target_include_directories(router.webui.html_files PUBLIC ${OUTPUT_DIR})
