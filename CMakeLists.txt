cmake_minimum_required(VERSION 3.9)
project(soi_vfs)

set(CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.58.0 COMPONENTS filesystem system program_options)

if(Boost_FOUND)
    add_executable(soi_vfs main.cpp FileSystem.cpp FileSystem.h)
    target_link_libraries(soi_vfs Boost::filesystem Boost::system Boost::program_options)
endif()