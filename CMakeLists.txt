cmake_minimum_required(VERSION 3.28)
project(datscity)

set(CMAKE_CXX_STANDARD 23)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_executable(datscity main.cpp)
add_definitions("-g -Ofast")

find_package(OpenGL REQUIRED)

# Try to find SDL2 package instead of hardcoding paths
find_package(SDL2 REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

# Link libraries
target_link_libraries(datscity
        ${OPENSSL_LIBRARIES}
        ${OPENGL_LIBRARIES}
        SDL2::SDL2
)

# Windows-specific linking (if you are targeting Windows)
if(WIN32)
    target_link_libraries(datscity wsock32 crypt32 ws2_32)
endif()
