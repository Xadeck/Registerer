cmake_minimum_required (VERSION 2.8)

project (registerer C CXX)
enable_testing()
include(GMock)
include(Cxx11)

add_executable(registerer_test registerer_test.cc registerer_test_deps.h registerer_test_deps.cc)
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
