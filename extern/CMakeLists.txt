add_library(extern_deps INTERFACE)

add_library(glad STATIC "glad/src/glad.c")
target_include_directories(glad PUBLIC "glad/include")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "stb")

if(WIN32)
    add_subdirectory(glfw)

    add_library(imgui STATIC
        "imgui/imgui.cpp"
        "imgui/imgui_demo.cpp"
        "imgui/imgui_draw.cpp"
        "imgui/imgui_tables.cpp"
        "imgui/imgui_widgets.cpp"
        "imgui/backends/imgui_impl_glfw.cpp"
        "imgui/backends/imgui_impl_opengl3.cpp"
    )
    target_include_directories(imgui PUBLIC "imgui")
    target_link_libraries(imgui PRIVATE glfw)

    target_link_libraries(extern_deps INTERFACE glfw glad winmm stb imgui)
    target_compile_definitions(extern_deps INTERFACE IMGUI)

else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DRM REQUIRED libdrm)
    pkg_check_modules(GBM REQUIRED gbm)
    pkg_check_modules(EGL REQUIRED egl)
    pkg_check_modules(GLES2 REQUIRED glesv2)
    pkg_check_modules(ALSA REQUIRED alsa)

    target_include_directories(extern_deps INTERFACE
        ${DRM_INCLUDE_DIRS}
        ${GBM_INCLUDE_DIRS}
        ${EGL_INCLUDE_DIRS}
        ${GLES2_INCLUDE_DIRS}
        ${ALSA_INCLUDE_DIRS}
    )

    target_link_libraries(extern_deps INTERFACE
        ${DRM_LIBRARIES}
        ${GBM_LIBRARIES}
        ${EGL_LIBRARIES}
        ${GLES2_LIBRARIES}
        ${ALSA_LIBRARIES}
        glad
        stb
    )
endif()
