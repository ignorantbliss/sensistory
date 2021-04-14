# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("sharding")

# Include sub-projects.
add_subdirectory ("src")
add_subdirectory ("src/sharddna")
add_subdirectory ("src/queryservice")
add_subdirectory ("src/core")
