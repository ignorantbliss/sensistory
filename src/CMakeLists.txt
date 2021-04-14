# CMakeList.txt : CMake project for Sharding, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

add_definitions(/bigobj)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options log)

# Add source to this project's executable.
add_executable (sharding "main.cpp" "sharding.h" "core/core.cpp" "queryservice/httpservercommon.cpp" "sharddna/History.cpp" "sharddna/DataStore.cpp"  "sharddna/LogListener.cpp" "sharddna/ConfigFile.cpp" "sharddna/Cursor.cpp" "sharddna/BTree.cpp" "sharddna/BTreeNode.cpp" "sharddna/BTreeKey.cpp" "sharddna/TSPoint.cpp" "sharddna/ShardCursor.cpp" "sharddna/QueryCursor.cpp" "sharddna/Series.cpp")
target_include_directories(sharding PRIVATE ${SQLite3_INCLUDE_DIRS})
target_include_directories(sharding PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(sharding ${SQLite3_LIBRARIES}  ${Boost_LIBRARIES})
install(TARGETS sharding DESTINATION ${CMAKE_BINARY_DIR}/bin)

# TODO: Add tests and install targets if needed.
