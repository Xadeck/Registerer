cmake_minimum_required (VERSION 3.1)

project (registerer)

##################################
# Download and install GoogleTest

include(ExternalProject)
ExternalProject_Add(gtest
  URL https://googletest.googlecode.com/files/gtest-1.7.0.zip
  # Comment above line, and uncomment line below to use subversion.
  # SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/ 
  # Uncomment line below to freeze a revision (here the one for 1.7.0)
  # SVN_REVISION -r700
  
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gtest source_dir binary_dir)

add_executable(registerer_test registerer_test.cc)

add_dependencies(registerer_test gtest)
include_directories(${source_dir}/include)
target_link_libraries(registerer_test ${binary_dir}/libgtest.a)
target_link_libraries(registerer_test ${binary_dir}/libgtest_main.a)

enable_testing()
add_test(NAME    registerer_test 
         COMMAND registerer_test)