set(CMAKE_CXX_STANDARD 20)

cmake_minimum_required(VERSION 3.15)
project(HFT CXX)

# Set the Boost policy to NEW before finding Boost
cmake_policy(SET CMP0167 NEW)

# Find Boost components
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(OpenSSL REQUIRED)

option(HFT_ENABLE_TESTS "Build unit tests" OFF)

# Add the executable with the actual source files
add_executable(HFT
        main.cpp
)


# Add include directories
target_include_directories(HFT PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/GatewayIn/include
        ${CMAKE_CURRENT_SOURCE_DIR}/GatewayIn/src
)

# Link Boost libraries
target_link_libraries(HFT PRIVATE
        Boost::system
        Boost::thread
        Boost::filesystem
        OpenSSL::SSL
        OpenSSL::Crypto
)

if (HFT_ENABLE_TESTS)
    include(FetchContent)

    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    add_executable(test_parser
            tests/parser/test_bitvavo_parser.cpp
            tests/parser/test_fix_parser.cpp
    )
    target_link_libraries(test_parser PRIVATE GTest::gmock_main)
    target_include_directories(test_parser PRIVATE
            ${CMAKE_SOURCE_DIR}/parser/include
            ${CMAKE_SOURCE_DIR}/gateway/include
    )
    gtest_discover_tests(test_parser)

    set(MOCKS_DIR ${CMAKE_SOURCE_DIR}/tests/mocks)
    add_executable(test_gateway
            tests/gateway/test_quotes_obtainer.cpp
    )
    target_link_libraries(test_gateway PRIVATE
            GTest::gmock_main)
    target_include_directories(test_gateway PRIVATE
            ${MOCKS_DIR}/GatewayIn/include
            ${CMAKE_SOURCE_DIR}/GatewayIn/include
            ${CMAKE_SOURCE_DIR}/GatewayIn/src
            ${CMAKE_SOURCE_DIR}/Parser/include
            ${Boost_INCLUDE_DIRS}
    )
    gtest_discover_tests(test_gateway)

    add_custom_target(run-tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS test_parser test_gateway
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running all unit tests"
    )
endif()
