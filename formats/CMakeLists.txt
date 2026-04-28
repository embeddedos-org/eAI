add_library(eai_formats STATIC
    src/gguf_loader.c
    src/onnx_loader.c
    src/format_registry.c
)

target_include_directories(eai_formats PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(eai_formats PUBLIC eai_common eai_accel)
