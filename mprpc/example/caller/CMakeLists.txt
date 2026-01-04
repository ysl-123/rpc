# CMakeLists.txt for example/caller

# 指定最小 CMake 版本
cmake_minimum_required(VERSION 3.0)

# 定义可执行文件名称
set(TARGET_NAME consumer)

# 定义源文件列表
# calluserservice.cc 是你的业务逻辑，user.pb.cc 是 protobuf 自动生成文件
#假如两者里面都没有main函数的话，最后可以单独写一个main函数的文件，想用哪个选择哪个就可以直接放一起编译，但是现在有两个main所以执行不了
#第一种calluserservice.cc
#set(SRC_LIST calluserservice.cc ../user.pb.cc)
#第二种方式callfriendservice
set(SRC_LIST     callfriendservice.cc     ../friend.pb.cc)

# 查找 protobuf 库（自己安装的在 /usr/local）
find_library(PROTOBUF_LIB NAMES protobuf PATHS /usr/local/lib)
find_path(PROTOBUF_INCLUDE google/protobuf/message.h PATHS /usr/local/include)

# 添加 include 路径
include_directories(${PROTOBUF_INCLUDE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)  # 包含 example 目录，user.pb.h 在这里

# 生成可执行文件
add_executable(${TARGET_NAME} ${SRC_LIST})

# 链接库：mprpc 和 protobuf
# 确保使用绝对路径的 protobuf 库
target_link_libraries(${TARGET_NAME} mprpc ${PROTOBUF_LIB})
