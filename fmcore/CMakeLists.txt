cmake_minimum_required(VERSION 3.16)
project(fmcore_multi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# --- Read Config.cmake to find custom output directories and deps directory ---
if(EXISTS "${CMAKE_SOURCE_DIR}/Config.cmake")
    include(${CMAKE_SOURCE_DIR}/Config.cmake)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(OPENCV_STATIC_ROOT "${OPENCV_BASE_PATH}/android")
    set(ONNXRUNTIME_DYNAMIC_ROOT "${ONNXRUNTIME_BASE_PATH}/android")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(OPENCV_STATIC_ROOT "${OPENCV_BASE_PATH}/ios")
    set(ONNXRUNTIME_DYNAMIC_ROOT "${ONNXRUNTIME_BASE_PATH}/ios")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OPENCV_STATIC_ROOT "${OPENCV_BASE_PATH}/linux")
    set(ONNXRUNTIME_DYNAMIC_ROOT "${ONNXRUNTIME_BASE_PATH}/linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_CROSSCOMPILING)
    set(OPENCV_STATIC_ROOT "${OPENCV_BASE_PATH}/macos")
    set(ONNXRUNTIME_DYNAMIC_ROOT "${ONNXRUNTIME_BASE_PATH}/macos")
endif()

# --- Helper to link static and dynamic dependencies ---
function(link_deps target)
    if(NOT DEFINED OPENCV_STATIC_ROOT)
        message(FATAL_ERROR "OPENCV_STATIC_ROOT must be defined")
    endif()
    if(NOT DEFINED ONNXRUNTIME_DYNAMIC_ROOT)
        message(FATAL_ERROR "ONNXRUNTIME_DYNAMIC_ROOT must be defined")
    endif()

    set(OPENCV_INCLUDE_DIR ${OPENCV_BASE_PATH}/include)
    set(OPENCV_LIBS_DIR ${OPENCV_STATIC_ROOT})
    set(ONNX_INCLUDE_DIR ${ONNXRUNTIME_BASE_PATH}/include)
    set(ONNX_LIB_DIR ${ONNXRUNTIME_DYNAMIC_ROOT})
    
    target_include_directories(${target} PUBLIC ${ONNX_INCLUDE_DIR})
    target_include_directories(${target} PUBLIC ${OPENCV_INCLUDE_DIR})
    target_link_libraries(${target}
        PUBLIC
            ${OPENCV_LIBS_DIR}/libopencv_core.a
            ${OPENCV_LIBS_DIR}/libopencv_imgcodecs.a
            ${OPENCV_LIBS_DIR}/libopencv_imgproc.a
            ${OPENCV_LIBS_DIR}/libopencv_highgui.a
            ${OPENCV_LIBS_DIR}/liblibjpeg-turbo.a
            ${OPENCV_LIBS_DIR}/liblibtiff.a
            ${OPENCV_LIBS_DIR}/liblibpng.a
            ${OPENCV_LIBS_DIR}/libzlib.a
            onnxruntime
    )
endfunction()

# --- Helper function for copying built lib and headers to custom output dirs ---
function(copy_library_to lib_var dest_dir)
    if(EXISTS ${dest_dir})
        add_custom_command(TARGET ${lib_var} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${lib_var}> ${dest_dir}
            COMMAND ${CMAKE_COMMAND} -E copy ${HEADER_LIB_PATH} ${dest_dir}/include
            COMMENT "Copying ${lib_var} to ${dest_dir} and ${HEADER_LIB_PATH} to ${dest_dir}/include"
        )
    endif()
endfunction()

# --- Helper function for copying header to custom output dirs ---
function(copy_headers_to dest_dir)
    if(EXISTS ${dest_dir})
        # Sanitize path to generate a valid target name (replacing / with _)
        string(REPLACE "/" "_" target_name ${dest_dir})
        string(REPLACE ":" "_" target_name ${target_name})

        add_custom_target(copy_headers_${target_name} ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory ${dest_dir}/include
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include ${dest_dir}/include
            COMMENT "Copying headers to ${dest_dir}/include"
        )
    endif()
endfunction()


# --- Sources ---
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Android build (only if toolchain defines Android platform) ---
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    message(STATUS "Configuring Android build")
    
    message(STATUS "Building fmcore for Android ABI=${CMAKE_ANDROID_ARCH_ABI}")
    add_library(fmcore_android_${CMAKE_ANDROID_ARCH_ABI} STATIC ${SOURCES})
    target_include_directories(fmcore_android_${CMAKE_ANDROID_ARCH_ABI} PUBLIC ${INCLUDES})
    target_compile_definitions(fmcore_android_${CMAKE_ANDROID_ARCH_ABI} PRIVATE __ANDROID__)
    set_target_properties(fmcore_android_${CMAKE_ANDROID_ARCH_ABI} PROPERTIES
       OUTPUT_NAME           fmcore_android_${CMAKE_ANDROID_ARCH_ABI}
       ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/android/${CMAKE_ANDROID_ARCH_ABI}"
       COMPILE_OPTIONS "-D__ANDROID__"
    )
    link_deps(fmcore_android_${CMAKE_ANDROID_ARCH_ABI})
    
    if(DEFINED EXTRA_OUTPUT_ANDROID)
        copy_library_to(fmcore_android_${CMAKE_ANDROID_ARCH_ABI} "${EXTRA_OUTPUT_ANDROID}")
    endif()
endif()

# --- iOS build (only if toolchain is targeting iOS) ---
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(STATUS "Configuring iOS build")

    execute_process(
        COMMAND xcrun --sdk iphoneos --show-sdk-path
        OUTPUT_VARIABLE IOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    add_library(fmcore_ios_arm64 STATIC ${SOURCES})
    target_include_directories(fmcore_ios_arm64 PUBLIC ${INCLUDES})
    set_target_properties(fmcore_ios_arm64 PROPERTIES
        OUTPUT_NAME fmcore_ios_arm64
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/ios"
    )
    target_compile_options(fmcore_ios_arm64 PRIVATE
        -target arm64-apple-ios
        -isysroot ${IOS_SDK_PATH}
    )

    link_deps(fmcore_ios_arm64)

    if(DEFINED EXTRA_OUTPUT_IOS)
        copy_library_to(fmcore_ios_arm64 "${EXTRA_OUTPUT_IOS}")
    endif()
endif()

# --- macOS ARM64 native build ---
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Configuring macOS ARM64 build")

    add_library(fmcore_macos_arm64 STATIC ${SOURCES})
    target_include_directories(fmcore_macos_arm64 PUBLIC ${INCLUDES})
    set_target_properties(fmcore_macos_arm64 PROPERTIES
        OUTPUT_NAME fmcore_macos_arm64
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/macos"
    )
    target_compile_options(fmcore_macos_arm64 PRIVATE
        -target arm64-apple-macos
        -arch arm64
    )

    link_deps(fmcore_macos_arm64)
    
    # Link Cocoa and Foundation for cv::imshow
    target_link_libraries(fmcore_macos_arm64
        PRIVATE
            "-framework AppKit"
            "-framework Foundation"
    )
    
    target_compile_definitions(fmcore_macos_arm64 PRIVATE FMCORE_NATIVE_BUILD)

    if(DEFINED EXTRA_OUTPUT_NATIVE)
        copy_library_to(fmcore_macos_arm64 "${EXTRA_OUTPUT_NATIVE}")
    endif()
endif()

# --- Linux native build ---
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configuring Linux x86_64 build")

    add_library(fmcore_linux_x86_64 STATIC ${SOURCES})
    target_include_directories(fmcore_linux_x86_64 PUBLIC ${INCLUDES})
    set_target_properties(fmcore_linux_x86_64 PROPERTIES
        OUTPUT_NAME fmcore_linux_x86_64
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/linux"
    )

    link_deps(fmcore_linux_x86_64)
    
    target_compile_definitions(fmcore_linux_x86_64 PRIVATE FMCORE_NATIVE_BUILD)

    if(DEFINED EXTRA_OUTPUT_NATIVE)
        copy_library_to(fmcore_linux_x86_64 "${EXTRA_OUTPUT_NATIVE}")
    endif()
endif()

# --- Native test executable (only for macOS or Linux host builds) ---
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Building native test binary for macOS")
    add_executable(fmcore_test test/FMCoreTest.cpp)
    target_include_directories(fmcore_test PRIVATE ${INCLUDES})
    target_link_libraries(fmcore_test PRIVATE fmcore_macos_arm64 onnxruntime)
    target_link_directories(fmcore_test PRIVATE ${ONNXRUNTIME_DYNAMIC_ROOT})

elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Building native test binary for Linux")
    add_executable(fmcore_test test/FMCoreTest.cpp)
    target_include_directories(fmcore_test PRIVATE ${INCLUDES})
    target_link_libraries(fmcore_test PRIVATE fmcore_linux_x86_64 onnxruntime)
    target_link_directories(fmcore_test PRIVATE ${ONNXRUNTIME_DYNAMIC_ROOT})
endif()
