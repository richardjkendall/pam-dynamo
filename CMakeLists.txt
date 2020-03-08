# minimal CMakeLists.txt for the AWS SDK for C++
cmake_minimum_required(VERSION 3.2)

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
add_library(pam-dynamo SHARED main.cpp User.cpp log.cpp)

# link the libraries we need
target_link_libraries(pam-dynamo ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(pam-dynamo OpenSSL::SSL)
target_link_libraries(pam-dynamo ${PAM_LIBRARIES})

# set install target
install(TARGETS pam-dynamo
    COMPONENT libraries
    DESTINATION "/lib/x86_64-linux-gnu/security/"
)

# packaging instructions
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "rjk") #required
INCLUDE(CPack)