cmake_minimum_required(VERSION 3.31)

project("SeaBattle" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Widgets Core REQUIRED)
qt_standard_project_setup()

add_subdirectory("src")
