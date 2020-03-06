# minimal CMakeLists.txt for the AWS SDK for C++
cmake_minimum_required(VERSION 3.2)

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_CXX_STANDARD 11)

# "my-example" is just an example value.
project(pam-dynamo)

# Locate the AWS SDK for C++ package.
# Requires that you build with:
#   -DCMAKE_PREFIX_PATH=/path/to/sdk_install
find_package(AWSSDK REQUIRED COMPONENTS dynamodb)

# find openssl
find_package(OpenSSL REQUIRED)

# find PAM
find_package(PAM REQUIRED)

# The library name and its sourcefiles
add_library(pam-dynamo SHARED main.cpp user.cpp log.cpp)

# The libraries used by your executable.
# "aws-cpp-sdk-s3" is just an example.
target_link_libraries(pam-dynamo ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(pam-dynamo OpenSSL::SSL)
target_link_libraries(pam-dynamo ${PAM_LIBRARIES})