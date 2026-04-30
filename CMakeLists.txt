cmake_minimum_required(VERSION 3.16)
project(EAI
    VERSION 0.2.0
    DESCRIPTION "AI Layer — Embedded AI platform for intelligent systems"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ========== Core module options ==========
option(EAI_BUILD_MIN       "Build EAI-Min lightweight runtime"       ON)
option(EAI_BUILD_FRAMEWORK "Build EAI-Framework industrial platform" ON)
option(EAI_BUILD_BCI       "Build EAI-BCI brain-computer interface"  ON)
option(EAI_BUILD_CLI       "Build EAI CLI tool"                      ON)
option(EAI_BUILD_TESTS     "Build unit tests"                        OFF)
option(EAI_BUILD_ACCEL     "Build accelerator module"                ON)
option(EAI_BUILD_FORMATS   "Build model format loaders"              ON)
option(EAI_BUILD_TOOLS     "Build developer tools"                   OFF)
option(EAI_BUILD_SHARED    "Build shared library (libeai.so/dll)"    OFF)

# ========== Platform options ==========
option(EAI_PLATFORM_EOS       "Enable EoS platform adapter"       OFF)
option(EAI_PLATFORM_LINUX     "Enable Linux platform adapter"     ON)
option(EAI_PLATFORM_WINDOWS   "Enable Windows platform adapter"   OFF)
option(EAI_PLATFORM_CONTAINER "Enable container platform adapter" OFF)
option(EAI_PLATFORM_MACOS     "Enable macOS platform adapter"     OFF)
option(EAI_PLATFORM_ANDROID   "Enable Android platform adapter"   OFF)
option(EAI_PLATFORM_IOS       "Enable iOS platform adapter"       OFF)
option(EAI_PLATFORM_ZEPHYR    "Enable Zephyr RTOS adapter"        OFF)
option(EAI_PLATFORM_FREERTOS  "Enable FreeRTOS adapter"           OFF)
option(EAI_PLATFORM_BAREMETAL "Enable bare-metal adapter"         OFF)

# ========== Accelerator options ==========
option(EAI_ACCEL_VULKAN "Enable Vulkan Compute backend"  OFF)
option(EAI_ACCEL_COREML "Enable CoreML/Metal backend"    OFF)
option(EAI_ACCEL_QNN    "Enable Qualcomm QNN backend"    OFF)

# ========== Other options ==========
option(EAI_EIPC_ENABLED "Enable EIPC integration for ENI→EAI communication" OFF)
option(EAI_LLAMA_CPP "Enable real llama.cpp runtime backend" OFF)
option(EAI_BUILD_ADAPTIVE "Enable adaptive/personalized AI features" ON)

if(EAI_EIPC_ENABLED)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../eipc/sdk/c ${CMAKE_BINARY_DIR}/eipc_c_sdk EXCLUDE_FROM_ALL)
endif()

# ========== Auto-detect platform ==========
if(WIN32)
    set(EAI_PLATFORM_LINUX OFF)
    set(EAI_PLATFORM_WINDOWS ON)
endif()

if(APPLE)
    set(EAI_PLATFORM_LINUX OFF)
    set(EAI_PLATFORM_MACOS ON)
    enable_language(OBJC)
endif()

if(ANDROID)
    set(EAI_PLATFORM_LINUX OFF)
    set(EAI_PLATFORM_ANDROID ON)
endif()

# ========== Modules ==========

# Common library (shared contracts + utilities)
add_subdirectory(common)

# Platform adapters
add_subdirectory(platform)

if(EAI_BUILD_MIN)
    add_subdirectory(min)
endif()

if(EAI_BUILD_FRAMEWORK)
    add_subdirectory(framework)
endif()

if(EAI_BUILD_BCI)
    add_subdirectory(bci)
endif()

add_subdirectory(models)

if(EAI_BUILD_ACCEL)
    add_subdirectory(accel)
endif()

if(EAI_BUILD_FORMATS)
    add_subdirectory(formats)
endif()

if(EAI_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(EAI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ========== Install targets ==========
include(GNUInstallDirs)
install(TARGETS eai_common eai_platform
    EXPORT eai-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY common/include/eai DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY platform/include/eai DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(EAI_BUILD_ACCEL)
    install(TARGETS eai_accel EXPORT eai-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY accel/include/eai DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(EAI_BUILD_FORMATS)
    install(TARGETS eai_formats EXPORT eai-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY formats/include/eai DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

install(EXPORT eai-targets
    FILE eai-config.cmake
    NAMESPACE eai::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eai
)

# ========== Status ==========
message(STATUS "--------------------------------------------")
message(STATUS "EAI v${PROJECT_VERSION}")
message(STATUS "  EAI-Min:       ${EAI_BUILD_MIN}")
message(STATUS "  EAI-Framework: ${EAI_BUILD_FRAMEWORK}")
message(STATUS "  EAI-BCI:       ${EAI_BUILD_BCI}")
message(STATUS "  Accelerator:   ${EAI_BUILD_ACCEL}")
message(STATUS "  Formats:       ${EAI_BUILD_FORMATS}")
message(STATUS "  CLI:           ${EAI_BUILD_CLI}")
message(STATUS "  Platform:      eos=${EAI_PLATFORM_EOS} linux=${EAI_PLATFORM_LINUX} win=${EAI_PLATFORM_WINDOWS} macos=${EAI_PLATFORM_MACOS} android=${EAI_PLATFORM_ANDROID} container=${EAI_PLATFORM_CONTAINER}")
message(STATUS "  Accel:         vulkan=${EAI_ACCEL_VULKAN} coreml=${EAI_ACCEL_COREML} qnn=${EAI_ACCEL_QNN}")
message(STATUS "--------------------------------------------")
