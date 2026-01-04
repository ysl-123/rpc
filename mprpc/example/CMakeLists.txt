add_subdirectory(callee)
add_subdirectory(caller)
# 让编译器和 VSCode 知道 protobuf 和 user.pb.h 的位置
include_directories(
    ${CMAKE_SOURCE_DIR}/example      # user.pb.h
    /usr/local/include               # protobuf
)