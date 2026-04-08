cmake_minimum_required(VERSION 3.16)
project(NightLife3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────────────────────
#  Library path   same as your previous assignments
#  Change this to match your computer if needed
# ─────────────────────────────────────────────────────────────────────────────
set(LIBRARY_PATH "D:/computergraphics/Libraries")

# ─────────────────────────────────────────────────────────────────────────────
#  Place exe in project root so "shaders/" relative path works
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# ─────────────────────────────────────────────────────────────────────────────
#  Sources
# ─────────────────────────────────────────────────────────────────────────────
add_executable(NightLife3D
    main.cpp
    src/globals.cpp
    src/renderer.cpp
    src/scene.cpp
    src/camera.cpp
    include/globals.h
    include/renderer.h
    include/scene.h
    include/camera.h
)

# ─────────────────────────────────────────────────────────────────────────────
#  Include dirs
# ─────────────────────────────────────────────────────────────────────────────
target_include_directories(NightLife3D PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${LIBRARY_PATH}/include
)

# ─────────────────────────────────────────────────────────────────────────────
#  OpenGL
# ─────────────────────────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)
target_link_libraries(NightLife3D PRIVATE OpenGL::GL)

# ─────────────────────────────────────────────────────────────────────────────
#  Windows: freeglut + glew32
# ─────────────────────────────────────────────────────────────────────────────
if(WIN32)
    target_link_libraries(NightLife3D PRIVATE
        "${LIBRARY_PATH}/lib/glew32.lib"
        "${LIBRARY_PATH}/lib/freeglut.lib"
    )

    add_custom_command(TARGET NightLife3D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBRARY_PATH}/bin/glew32.dll"
            $<TARGET_FILE_DIR:NightLife3D>
    )
    add_custom_command(TARGET NightLife3D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBRARY_PATH}/bin/freeglut.dll"
            $<TARGET_FILE_DIR:NightLife3D>
    )
endif()
