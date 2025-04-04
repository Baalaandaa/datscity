cmake_minimum_required(VERSION 3.28)
project(datscity)

set(CMAKE_CXX_STANDARD 23)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(datscity main.cpp viewer/ActiveSocket.cpp viewer/HTTPActiveSocket.cpp viewer/PassiveSocket.cpp viewer/SimpleSocket.cpp)


include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(datscity ${OPENSSL_LIBRARIES})
if(WIN32)
    target_link_libraries(datscity wsock32 crypt32 ws2_32)
endif()