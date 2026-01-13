set(GAME_INCLUDE_DIRS)
set(GAME_LIBRARIES)

add_library(extern_deps INTERFACE)

add_library(glad STATIC "glad/src/glad.c")
target_include_directories(glad PUBLIC "glad/include")

if(WIN32)
    add_subdirectory(glfw)
    target_link_libraries(extern_deps INTERFACE glfw glad winmm)

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
    )
endif()
