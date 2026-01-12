cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(JuceHelloWorld VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(VCPKG_ACTIVE FALSE)
set(NEEDS_WEBVIEW2_VALUE TRUE)
if(NOT VCPKG_ACTIVE)
    if(DEFINED VCPKG_ROOT OR DEFINED VCPKG_TARGET_TRIPLET OR DEFINED VCPKG_HOST_TRIPLET)
        set(VCPKG_ACTIVE TRUE)
        set(NEEDS_WEBVIEW2_VALUE FALSE)
    endif()
endif()
add_subdirectory(libs/juce)

set(WEBVIEW_UI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/react-hello/dist")
set(WEBVIEW_FILES_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

set(JUCE_WEBVIEW_SRC_DIR "${JUCE_MODULES_DIR}/juce_gui_extra/native/javascript")
set(JUCE_WEBVIEW_DST_DIR "${WEBVIEW_FILES_DST_DIR}/js/juce")

set(WEBVIEW_FILES_ZIP_NAME "webview_files.zip")
set(TARGET_WEBVIEW_FILES_ZIP_PATH "${CMAKE_BINARY_DIR}/${WEBVIEW_FILES_ZIP_NAME}")

file(GLOB_RECURSE JUCE_WEBVIEW_SRC_FILES
    CONFIGURE_DEPENDS
    "${JUCE_WEBVIEW_SRC_DIR}/*"
)

file(GLOB_RECURSE WEBVIEW_UI_SRC_FILES
    CONFIGURE_DEPENDS
    "${WEBVIEW_UI_SOURCE_DIR}/*"
)

set(COPIED_WEBVIEW_FILES "")
foreach(src ${WEBVIEW_UI_SRC_FILES})
    file(RELATIVE_PATH rel "${WEBVIEW_UI_SOURCE_DIR}" "${src}")
    set(dst "${WEBVIEW_FILES_DST_DIR}/${rel}")
    get_filename_component(dst_dir "${dst}" DIRECTORY)
    add_custom_command(
        OUTPUT "${dst}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${dst_dir}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${src}" "${dst}"
        DEPENDS "${src}"
        COMMENT "Copy (if changed): ${rel} ${src} -> ${dst}"
        VERBATIM
    )
    list(APPEND COPIED_WEBVIEW_FILES "${dst}")
endforeach()

add_custom_target(CopyWebViewFiles
    DEPENDS ${COPIED_WEBVIEW_FILES}
    COMMENT "Synchronizing WebView files to ${WEBVIEW_DST_DIR}"
)

add_custom_command(
    OUTPUT "${TARGET_WEBVIEW_FILES_ZIP_PATH}"
    COMMAND ${CMAKE_COMMAND} -E tar cvf ${TARGET_WEBVIEW_FILES_ZIP_PATH} --format=zip ${WEBVIEW_FILES_DST_DIR}
    DEPENDS ${COPIED_WEBVIEW_FILES}
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    COMMENT "Zipping WebView files to ${TARGET_WEBVIEW_FILES_ZIP_PATH}"
    VERBATIM
)

add_custom_target(WebViewFilesZip
    DEPENDS "${TARGET_WEBVIEW_FILES_ZIP_PATH}"
)

juce_add_binary_data(WebViewFiles
    HEADER_NAME WebViewFiles.h
    NAMESPACE webview_files
    SOURCES ${TARGET_WEBVIEW_FILES_ZIP_PATH}
)

add_dependencies(WebViewFilesZip CopyWebViewFiles)
add_dependencies(WebViewFiles WebViewFilesZip)

juce_add_gui_app(JuceHelloWorld
    PRODUCT_NAME "ExampleWebView"
    NEEDS_WEBVIEW2 ${NEEDS_WEBVIEW2_VALUE}
)

# Pass the prefix of the zipped files (e.g., "assets/") to C++
cmake_path(GET WEBVIEW_FILES_DST_DIR FILENAME ZIPPED_FILES_PREFIX)
target_compile_definitions(JuceHelloWorld PRIVATE ZIPPED_FILES_PREFIX="${ZIPPED_FILES_PREFIX}/")

target_compile_features(JuceHelloWorld PRIVATE cxx_std_20)
target_sources(JuceHelloWorld PRIVATE main.cpp)

target_link_libraries(JuceHelloWorld
    PRIVATE
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_events
    juce::juce_data_structures
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    WebViewFiles
)

if(VCPKG_ACTIVE)
    find_package(unofficial-webview2 CONFIG REQUIRED)
    target_link_libraries(
        JuceHelloWorld PRIVATE unofficial::webview2::webview2
    )
endif()

target_compile_definitions(JuceHelloWorld PUBLIC
    JUCE_WINDOWS=1
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)
