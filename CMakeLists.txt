cmake_minimum_required (VERSION 3.0)

project (registerer C CXX)
add_definitions(-std=c++11)
enable_testing()
include(GMock)

add_executable(registerer_test registerer_test.cc)
add_gmock(registerer_test)
add_test(test registerer_test)

add_executable(registerer_example registerer_example.cc)

add_test(example_work registerer_example Circle red Rectangle green Rect yellow Ellipsis blue)
set_tests_properties(example_work
    PROPERTIES PASS_REGULAR_EXPRESSION "Circle\nRectangle\nRectangle\nEllipsis:blue"
)
add_test(example_fail registerer_example Unknown purple)
set_tests_properties(example_fail
    PROPERTIES PASS_REGULAR_EXPRESSION "No 'Unknown' shape registered"
)
