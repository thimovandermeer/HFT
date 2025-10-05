# src/Parser/CMakeLists.txt

add_library(Parser INTERFACE)

target_include_directories(Parser INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Parser INTERFACE
        Boost::filesystem
)
