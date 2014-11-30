cmake_minimum_required (VERSION 2.8)

project (registerer C CXX)
add_definitions(-std=c++11)
enable_testing()
include(GMock)

add_library(registerer INTERFACE)
target_sources(registerer INTERFACE registerer.h)

add_executable(registerer_test registerer_test.cc)
add_dependencies(registerer_test registerer)
add_gmock(registerer_test)
add_test(test registerer_test)

add_executable(registerer_example registerer_example.cc)
add_dependencies(registerer_example registerer)
add_test(example registerer_example Circle red Rectangle green Ellipsis blue)
