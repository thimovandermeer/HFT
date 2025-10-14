add_library(OrderBook
        src/OrderBook.cpp
)

target_include_directories(OrderBook PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(OrderBook PUBLIC GatewayIn)