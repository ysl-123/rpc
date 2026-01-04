#把当前目录下所有 .cpp/.c 文件收集起来，放到变量 SRC_LIST 里”
#aux_source_directory(. SRC_LIST)
set(SRC_LIST 
mprpcapplication.cc 
mprpcconfig.cc 
rpcheader.pb.cc 
rpcprovider.cc 
mprpcchannel.cc 
mprpccontroller.cc 
logger.cc 
zookeeperutil.cc)

#“用 SRC_LIST 里所有的源文件生成一个 名为 mprpc 的动态库”。
add_library(mprpc  ${SRC_LIST})

#写上依赖muduo库
target_link_libraries(mprpc muduo_net muduo_base  pthread zookeeper_mt)