add_executable(WindowDisplay "")

target_sources(WindowDisplay
PRIVATE
    main.cpp
    SimpleCapture.cpp
    SimpleCapture.h
    capture.interop.h
    composition.interop.h
    d3dHelpers.h
    direct3d11.interop.h
)

target_include_directories(WindowDisplay 
PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${DependenciesDirectory}/glm)

target_link_libraries(WindowDisplay 
PRIVATE
    SOIS
)

target_compile_definitions(WindowDisplay PRIVATE GLFW_INCLUDE_NONE)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    target_link_libraries(WindowDisplay PRIVATE opengl32.lib windowsapp.lib dwmapi.lib)

    target_compile_options(WindowDisplay PRIVATE 
    PUBLIC
        -permissive- -std:c++17
    PRIVATE
        -WX- -W4
    )
endif()

set_target_properties(WindowDisplay PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${AssetDirectory}/Bin)
