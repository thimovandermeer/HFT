cmake_minimum_required(VERSION 3.15)
project(HFT CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(OpenSSL REQUIRED)

option(HFT_ENABLE_TESTS "Build unit tests" OFF)

add_executable(HFT
        apps/hft/main.cpp
)

target_include_directories(HFT PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GatewayIn/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Parser/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Visualizer/include
)

target_link_libraries(HFT PRIVATE
        Boost::system
        Boost::thread
        Boost::filesystem
        OpenSSL::SSL
        OpenSSL::Crypto
)

if (HFT_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
