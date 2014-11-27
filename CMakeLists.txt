cmake_minimum_required (VERSION 2.8)

project (registerer C CXX)
find_package(Threads)
##################################
# Download and install GMock

include(ExternalProject)
ExternalProject_Add(gmock
  URL https://googlemock.googlecode.com/files/gmock-1.7.0.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gmock
  INSTALL_COMMAND ""
)

add_executable(registerer_test registerer_test.cc)
# TODO: the commands below should specify which features are needed
# but they fail on Mac so we use a hack to set a compiler flag.
#target_compile_features(registerer_test PUBLIC x`)
#target_compile_features(registerer_test PUBLIC cxx_lambdas)
#set_property(TARGET registerer_test PROPERTY CXX_STANDARD 11)
#set_property(TARGET registerer_test PROPERTY CXX_STANDARD_REQUIRED ON)
ADD_DEFINITIONS(
    -std=c++11
)

ExternalProject_Get_Property(gmock source_dir binary_dir)
add_dependencies(registerer_test gmock)
include_directories(${source_dir}/include)
include_directories(${source_dir}/gtest/include)
target_link_libraries(registerer_test ${binary_dir}/libgmock.a)
target_link_libraries(registerer_test ${binary_dir}/libgmock_main.a)
target_link_libraries(registerer_test -lpthread)

enable_testing()
add_test(NAME    registerer_test 
         COMMAND registerer_test)

