cmake_minimum_required (VERSION 2.6)

project (Registerer)

ExternalProject_Add(gtest
  URL https://googletest.googlecode.com/files/gtest-1.7.0.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=
)
ExternalProject_Get_Property(gtest gtest_dir)

include_directories(${gtest_dir}/include)

add_executable(registerer_test registerer_test.cc)