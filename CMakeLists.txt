set(CMAKE_CXX_STANDARD 20)

cmake_minimum_required(VERSION 3.15)
project(HFT CXX)

# Set the Boost policy to NEW before finding Boost
cmake_policy(SET CMP0167 NEW)

# Find Boost components
find_package(Boost REQUIRED COMPONENTS system thread filesystem)

# Add the executable with the actual source files
add_executable(HFT
        main.cpp
        GatewayIn/src/QuotesObtainer.cpp
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
)