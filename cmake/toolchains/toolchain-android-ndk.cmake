# Android NDK cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 26)  # API level
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
set(CMAKE_ANDROID_STL_TYPE c++_static)

# Set ANDROID_NDK via environment or CMake variable
if(DEFINED ENV{ANDROID_NDK_HOME})
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME})
endif()
