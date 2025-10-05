add_library(GatewayIn INTERFACE
)

target_include_directories(GatewayIn INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(GatewayIn INTERFACE
        Boost::system
        Boost::thread
        Boost::filesystem
        OpenSSL::SSL
        OpenSSL::Crypto
)

file(GLOB_RECURSE GATEWAY_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${GATEWAY_HEADERS})
