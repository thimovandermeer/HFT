# tests/CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

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

set(MOCKS_DIR ${CMAKE_SOURCE_DIR}/tests/mocks)

add_executable(HFT_tests
        parser/test_bitvavo_parser.cpp
        parser/test_fix_parser.cpp
        gateway/test_quotes_obtainer.cpp
        orderbook/test_quote_consumer.cpp
)

target_link_libraries(HFT_tests PRIVATE
        GatewayIn
        Parser
        Visualizer
        OrderBook
        GTest::gmock_main
        Boost::system
        Boost::thread
        Boost::filesystem
)

target_include_directories(HFT_tests PRIVATE
        ${MOCKS_DIR}/GatewayIn/include
        ${CMAKE_SOURCE_DIR}/src
)

gtest_discover_tests(HFT_tests DISCOVERY_MODE POST_BUILD)

add_custom_target(run-tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS HFT_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all unit tests"
)
