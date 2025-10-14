cmake_minimum_required(VERSION 3.20)
project(HFT CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(OpenSSL REQUIRED)

option(HFT_ENABLE_TESTS "Build unit tests" OFF)

add_subdirectory(src)

add_executable(HFT
        apps/hft/main.cpp
)

target_link_libraries(HFT PRIVATE
        GatewayIn
        Parser
        OrderBook
        Visualizer
        Boost::system
        Boost::thread
        Boost::filesystem
        OpenSSL::SSL
        OpenSSL::Crypto
)

if (HFT_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
