# CMakeLists.txt for example/callee

# 指定最小 CMake 版本
cmake_minimum_required(VERSION 3.0)

# 定义可执行文件名称
set(TARGET_NAME provider)

# 定义源文件列表
# userservice.cc 是你的业务逻辑，user.pb.cc 是 protobuf 自动生成文件
#set(SRC_LIST    userservice.cc      ../user.pb.cc)
 set(SRC_LIST    friendservice.cc    ../friend.pb.cc)

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
