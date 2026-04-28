add_executable(eai_test_common test_common.c)
target_link_libraries(eai_test_common PRIVATE eai_common)
add_test(NAME eai_common_tests COMMAND eai_test_common)

add_executable(eai_test_platform test_platform.c)
target_link_libraries(eai_test_platform PRIVATE eai_common eai_platform)
add_test(NAME eai_platform_tests COMMAND eai_test_platform)

add_executable(eai_test_hal_core test_hal_core.c)
target_link_libraries(eai_test_hal_core PRIVATE eai_common eai_platform)
add_test(NAME eai_hal_core_tests COMMAND eai_test_hal_core)

if(EAI_BUILD_ACCEL)
    add_executable(eai_test_accel test_accel.c)
    target_link_libraries(eai_test_accel PRIVATE eai_common eai_accel)
    if(UNIX)
        target_link_libraries(eai_test_accel PRIVATE m)
    endif()
    add_test(NAME eai_accel_tests COMMAND eai_test_accel)

    # --- Comprehensive tensor tests ---
    add_executable(eai_test_tensor test_tensor.c)
    target_link_libraries(eai_test_tensor PRIVATE eai_common eai_accel)
    add_test(NAME eai_tensor_tests COMMAND eai_test_tensor)

    # --- Comprehensive accel backend tests ---
    add_executable(eai_test_accel_backend test_accel_backend.c)
    target_link_libraries(eai_test_accel_backend PRIVATE eai_common eai_accel)
    if(UNIX)
        target_link_libraries(eai_test_accel_backend PRIVATE m)
    endif()
    add_test(NAME eai_accel_backend_tests COMMAND eai_test_accel_backend)
endif()

# --- Comprehensive format tests ---
if(EAI_BUILD_FORMATS)
    add_executable(eai_test_formats test_formats.c)
    target_link_libraries(eai_test_formats PRIVATE eai_common eai_accel eai_formats)
    add_test(NAME eai_format_tests COMMAND eai_test_formats)
endif()

# --- Comprehensive status/types tests ---
add_executable(eai_test_status test_status.c)
target_link_libraries(eai_test_status PRIVATE eai_common)
add_test(NAME eai_status_tests COMMAND eai_test_status)

# --- Comprehensive platform HAL tests ---
add_executable(eai_test_platform_hal test_platform_hal.c)
target_link_libraries(eai_test_platform_hal PRIVATE eai_common eai_platform)
add_test(NAME eai_platform_hal_tests COMMAND eai_test_platform_hal)

# --- Stable C API tests ---
add_executable(eai_test_eai_api test_eai_api.c)
target_link_libraries(eai_test_eai_api PRIVATE eai_common eai_platform)
add_test(NAME eai_api_tests COMMAND eai_test_eai_api)

# --- HAL subsystem tests (fs/thread/net) ---
add_executable(eai_test_hal_subsystems test_hal_subsystems.c)
target_link_libraries(eai_test_hal_subsystems PRIVATE eai_common eai_platform)
if(WIN32)
    target_link_libraries(eai_test_hal_subsystems PRIVATE ws2_32)
elseif(UNIX)
    target_link_libraries(eai_test_hal_subsystems PRIVATE pthread)
endif()
add_test(NAME eai_hal_subsystem_tests COMMAND eai_test_hal_subsystems)

# --- Extended format tests (GGUF numeric KV, tensors, format_load, ONNX multi-field) ---
if(EAI_BUILD_FORMATS)
    add_executable(eai_test_formats_ext test_formats_extended.c)
    target_link_libraries(eai_test_formats_ext PRIVATE eai_common eai_accel eai_formats)
    if(UNIX)
        target_link_libraries(eai_test_formats_ext PRIVATE m)
    endif()
    add_test(NAME eai_format_ext_tests COMMAND eai_test_formats_ext)
endif()

# --- Extended API tests (accel count/name, HAL-routed platform API) ---
add_executable(eai_test_api_extended test_api_extended.c)
target_link_libraries(eai_test_api_extended PRIVATE eai_common eai_platform)
add_test(NAME eai_api_extended_tests COMMAND eai_test_api_extended)

add_executable(eai_test_min test_min.c)
target_link_libraries(eai_test_min PRIVATE eai_common eai_min)
add_test(NAME eai_min_tests COMMAND eai_test_min)

add_executable(eai_test_framework test_framework.c)
target_link_libraries(eai_test_framework PRIVATE eai_common eai_framework)
add_test(NAME eai_framework_tests COMMAND eai_test_framework)

# --- test_tool: Tool registry ---
add_executable(eai_test_tool test_tool.c)
target_link_libraries(eai_test_tool PRIVATE eai_common)
add_test(NAME eai_tool_tests COMMAND eai_test_tool)

# --- test_config: Configuration ---
add_executable(eai_test_config test_config.c)
target_link_libraries(eai_test_config PRIVATE eai_common)
add_test(NAME eai_config_tests COMMAND eai_test_config)

# --- test_agent: Min agent ---
add_executable(eai_test_agent test_agent.c)
target_link_libraries(eai_test_agent PRIVATE eai_common eai_min)
add_test(NAME eai_agent_tests COMMAND eai_test_agent)

# --- test_memory: Memory lite ---
add_executable(eai_test_memory test_memory.c)
target_link_libraries(eai_test_memory PRIVATE eai_common eai_min)
add_test(NAME eai_memory_tests COMMAND eai_test_memory)

# --- test_models: Model catalog ---
add_executable(eai_test_models test_models.c)
target_link_libraries(eai_test_models PRIVATE eai_common)
add_test(NAME eai_model_tests COMMAND eai_test_models)

# --- test_adaptive: Adaptive/Personalized AI ---
add_executable(eai_test_adaptive test_adaptive.c)
target_link_libraries(eai_test_adaptive PRIVATE eai_common)
add_test(NAME eai_adaptive_tests COMMAND eai_test_adaptive)

# --- BCI tests ---
if(EAI_BUILD_BCI)
    add_executable(eai_test_bci_signal test_bci_signal.c)
    target_link_libraries(eai_test_bci_signal PRIVATE eai_common eai_platform eai_bci)
    add_test(NAME eai_bci_signal_tests COMMAND eai_test_bci_signal)

    add_executable(eai_test_bci_device test_bci_device.c)
    target_link_libraries(eai_test_bci_device PRIVATE eai_common eai_platform eai_bci)
    add_test(NAME eai_bci_device_tests COMMAND eai_test_bci_device)

    add_executable(eai_test_bci_pipeline test_bci_pipeline.c)
    target_link_libraries(eai_test_bci_pipeline PRIVATE eai_common eai_platform eai_bci)
    add_test(NAME eai_bci_pipeline_tests COMMAND eai_test_bci_pipeline)

    if(UNIX)
        target_link_libraries(eai_test_bci_signal PRIVATE m)
        target_link_libraries(eai_test_bci_device PRIVATE m)
        target_link_libraries(eai_test_bci_pipeline PRIVATE m)
    endif()
endif()
