cmake_minimum_required(VERSION 3.10)

# Project name
project(HW1_BMP CXX)

# Use C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

add_executable(HW1
    HW1.cpp
    bmp.cpp
)

target_link_libraries(HW1 PRIVATE ${OpenCV_LIBS})
target_include_directories(HW1 PRIVATE ${OpenCV_INCLUDE_DIRS})

add_executable(HW1_opencv
    HW1_opencv.cpp
)

target_link_libraries(HW1_opencv PRIVATE ${OpenCV_LIBS})
target_include_directories(HW1_opencv PRIVATE ${OpenCV_INCLUDE_DIRS})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(HW1 PRIVATE -Wall -Wextra -O2)
    target_compile_options(HW1_opencv PRIVATE -Wall -Wextra -O2)
endif()

# Silence MSVC warnings (optional for Windows)
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()
