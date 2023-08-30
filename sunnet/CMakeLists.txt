#项目名称
project(sunnet)

#cmake最低版本
cmake_minimum_required(VERSION 2.8)

#头文件目录
include_directories(include)

#查找./src目录下的所有源文件，保存到DIR_SRCS中
aux_source_directory(./src DIR_SRCS)

#添加库文件路径
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/3rd/lua-5.3.5/src/)


#用c++11
add_definitions(-std=c++11)

#指定生成目标文件
add_executable(sunnet ${DIR_SRCS})

#库文件
find_package(Threads)
target_link_libraries(sunnet ${camke_THREAD_LIBS_INIT})

#lua头文件、库
include_directories(./3rd/lua-5.3.5/src/)
target_link_libraries(sunnet liblua.a)
target_link_libraries(sunnet dl)

#开启多线程支持
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
