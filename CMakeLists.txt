cmake_minimum_required(VERSION 3.15)
project(ScreenRotator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(ScreenRotator WIN32
    main.cpp
    resource.rc
)

target_compile_definitions(ScreenRotator PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0601
    NTDDI_VERSION=0x06010000
)

target_link_libraries(ScreenRotator PRIVATE user32 shell32 gdi32)

if(MSVC)
    set_target_properties(ScreenRotator PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()
