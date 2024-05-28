cmake_minimum_required(VERSION 3.0)  #最低版本
project(main)  #定义当前工程名字

#配置编译选项
set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS}  -g)


#设置可执行文件最终存储的路径，PROJECT_SOURCE_DIR ->工程根目录，也就是当前的testmuduo
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#设置.so库和静态库放到这个里面，一般放到lib里面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#配置头文件搜索路径
#include_directories(${PROJECT_SOURCE_DIR}/include)


#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件列表
set(SRC_LIST test_muduo_server.cpp)   #设置SRC_LIST = test_muduo_server.cpp

#假设这个工程中有100个.cpp文件，把指定路径，这里是.(表示当前路径下的)的所有源文件名字放入变量名SRC_LIST
#aux_source_directory(. SRC_LIST)

#aux_source_directory(src SRC_LIST) #把当前src文件下的源文件名字放入变量名SRC_LIST

#生成可执行文件server, 由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST})   #相当于add_executable(server test_muduo_server.cpp) 

#表示server这个目标程序，需要连接muduo_net muduo_base pthread这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)