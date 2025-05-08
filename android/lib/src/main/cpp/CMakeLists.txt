
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("lib")




# Detect ABI (set by Android Gradle plugin)
set(ABI_DIR ${ANDROID_ABI})

# Path to third party libraries
set(DEPS_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/${ABI_DIR})
# Paths to third party includes
set(OPENCV_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(ONNX_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/onnxruntime)

# List OpenCV modules
if(${ANDROID_ABI} STREQUAL "arm64-v8a") # #TODO verify the library is working ofr other archs, kleidivc has been built only for arm64-v8a (due to opencv bug: https://github.com/opencv/opencv/issues/27158#issuecomment-2776291328)
    set(OPENCV_STATIC_LIBS
            opencv_core
            opencv_imgproc
            opencv_imgcodecs
            opencv_highgui
            cpufeatures
            kleidicv
            kleidicv_hal
            kleidicv_thread
            libtiff
            libpng
            libjpeg-turbo
            zlib
    )
else()
    set(OPENCV_STATIC_LIBS
            opencv_core
            opencv_imgproc
            opencv_imgcodecs
            opencv_highgui
            cpufeatures
            libtiff
            libpng
            libjpeg-turbo
            zlib
    )
endif()
# Declare each static OpenCV library as IMPORTED
foreach(lib ${OPENCV_STATIC_LIBS})
    add_library(${lib} STATIC IMPORTED)
    set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION ${DEPS_LIB_DIR}/lib${lib}.a
            INTERFACE_INCLUDE_DIRECTORIES ${OPENCV_INCLUDE_DIR}
    )
endforeach()
# Add ONNX Runtime shared library
add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION ${DEPS_LIB_DIR}/libonnxruntime.so
        INTERFACE_INCLUDE_DIRECTORIES ${ONNX_INCLUDE_DIR}
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
    # List C/C++ source files with relative paths to this CMakeLists.txt.
    lib.cpp)

add_library(libfmcore_android_${ANDROID_ABI} STATIC IMPORTED)
set_target_properties(libfmcore_android_${ANDROID_ABI} PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libfmcore_android_${ANDROID_ABI}.a)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
    # List libraries link to the target library
    android
    log libfmcore_android_${ANDROID_ABI} ${OPENCV_STATIC_LIBS} onnxruntime
)
