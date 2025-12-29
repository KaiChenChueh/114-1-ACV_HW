cmake_minimum_required(VERSION 3.10)

# Project name
project(HW3_BMP CXX)

# Use C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

add_executable(HW3
    HW3.cpp
    bmp.cpp
)

target_link_libraries(HW3 PRIVATE ${OpenCV_LIBS})
target_include_directories(HW3 PRIVATE ${OpenCV_INCLUDE_DIRS})

add_executable(HW3_opencv
    HW3_opencv.cpp
    bmp.cpp
)

target_link_libraries(HW3_opencv PRIVATE ${OpenCV_LIBS})
target_include_directories(HW3_opencv PRIVATE ${OpenCV_INCLUDE_DIRS})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(HW3 PRIVATE -Wall -Wextra -O2)
    target_compile_options(HW3_opencv PRIVATE -Wall -Wextra -O2)
endif()

# Silence MSVC warnings (optional for Windows)
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()
