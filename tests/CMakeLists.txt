
add_executable(test_parser
        parser/test_bitvavo_parser.cpp
        parser/test_fix_parser.cpp
)

target_link_libraries(test_parser PRIVATE
        parser
        gateway
        GTest::gtest_main
)

add_executable(test_gateway
        gateway/test_quotes_obtainer.cpp
)
target_link_libraries(test_gateway PRIVATE
        gateway
        parser
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_parser)
gtest_discover_tests(test_gateway)
