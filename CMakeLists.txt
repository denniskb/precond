cmake_minimum_required(VERSION 3.12)

project(pre)

add_executable(test
main.cpp)
set_target_properties(test PROPERTIES CXX_STANDARD 20)