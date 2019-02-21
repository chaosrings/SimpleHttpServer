cmake_minimum_required(VERSION 2.8)
project(myserver)
set(CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
#包含文件目录
include_directories(
    include
    /usr/local/include
    /usr/include/x86_64-linux-gnu
    /usr/include
)
include_directories(
    src
)
#库文件目录
link_directories(
    /usr/local/lib
    /usr/lib
    /usr/lib/x86_64-linux-gnu
)

# sub directories
# add_subdirectory()

#寻找所有的源文件
aux_source_directory(
    src/Socket SOURCE_DIR
)
aux_source_directory(
    src/Server SOURCE_DIR
)
aux_source_directory(
    src/Request  SOURCE_DIR
)
aux_source_directory(
    src/Response  SOURCE_DIR
)
aux_source_directory(
    src/Resource  SOURCE_DIR
)
aux_source_directory(
    src/DataVariant  SOURCE_DIR
)
aux_source_directory(
    src/ThreadPool  SOURCE_DIR
)
aux_source_directory(
    src/Buffer SOURCE_DIR
)
aux_source_directory(
    src/Reactor SOURCE_DIR
)
aux_source_directory(
    src/Utils  SOURCE_DIR
)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME} ${SOURCE_DIR} src/main.cpp)



#设置需要链接的lib，相当于vs里的输入lib
#libpthread  必须动态链接
target_link_libraries(${PROJECT_NAME} libpthread.so)