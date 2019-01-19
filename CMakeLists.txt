cmake_minimum_required(VERSION 3.12)
project(pattern_tracker)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  ")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

find_package(Qt5Widgets REQUIRED)

add_executable(pattern_tracker main.cpp main_window.h main_window.cpp main_window.ui directory_scanner/directory_scanner.cpp directory_scanner/directory_scanner.h directory_scanner/file_scanner/file_scanner.cpp directory_scanner/file_scanner/file_scanner.h searcher/searcher.cpp searcher/searcher.h)

target_link_libraries(pattern_tracker Qt5::Widgets)
target_link_libraries(pattern_tracker Qt5::Core)

include_directories("${CMAKE_BINARY_DIR}")