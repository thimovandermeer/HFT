cmake_minimum_required(VERSION 3.20)
include(FetchContent)


FetchContent_Declare(
        implot
        GIT_REPOSITORY https://github.com/epezent/implot.git
        GIT_TAG        v0.16
)
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
        ${implot_SOURCE_DIR}/implot.cpp
        ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

add_library(VisualizerImGui STATIC
        src/VisualizerImGui.cpp
)

target_include_directories(VisualizerImGui
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(Visualizer INTERFACE)

target_include_directories(Visualizer INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(Visualizer INTERFACE VisualizerImGui)


FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.0
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    set(IMGUI_SRC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    add_library(imgui STATIC ${IMGUI_SRC})
    target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC glfw)
endif()

target_link_libraries(VisualizerImGui PRIVATE implot imgui OrderBook)

if(APPLE)
    target_link_libraries(VisualizerImGui PRIVATE "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(VisualizerImGui PRIVATE OpenGL::GL)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_features(VisualizerImGui PRIVATE cxx_std_20)
    target_compile_options(VisualizerImGui PRIVATE -O2 -march=native -fno-omit-frame-pointer)
elseif(MSVC)
    target_compile_features(VisualizerImGui PRIVATE cxx_std_20)
    target_compile_options(VisualizerImGui PRIVATE /O2 /EHsc)
endif()
