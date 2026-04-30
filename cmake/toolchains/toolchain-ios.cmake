# iOS cross-compilation toolchain
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
set(CMAKE_OSX_ARCHITECTURES "arm64")

enable_language(OBJC)

# Disable features not available on iOS
set(EAI_HAL_GPIO OFF CACHE BOOL "" FORCE)
set(EAI_BUILD_CLI OFF CACHE BOOL "" FORCE)
