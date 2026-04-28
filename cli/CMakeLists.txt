add_executable(eai src/main.c)

target_link_libraries(eai PRIVATE
    eai_common
    eai_min
    eai_framework
    eai_platform
)

# Developer tools — link sources and dependencies
if(EAI_BUILD_TOOLS AND EAI_BUILD_ACCEL AND EAI_BUILD_FORMATS)
    target_compile_definitions(eai PRIVATE EAI_BUILD_TOOLS)
    target_sources(eai PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/bench/eai_bench.c
        ${CMAKE_SOURCE_DIR}/tools/convert/eai_convert.c
        ${CMAKE_SOURCE_DIR}/tools/profile/eai_profile.c
    )
    target_link_libraries(eai PRIVATE eai_accel eai_formats)
    target_include_directories(eai PRIVATE
        ${CMAKE_SOURCE_DIR}/accel/include
        ${CMAKE_SOURCE_DIR}/formats/include
    )
    if(UNIX)
        target_link_libraries(eai PRIVATE m)
    endif()
endif()
