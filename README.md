# chatserver
可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端源码 基于muduo实习

编译方式
cd build
rm -rf *
cmake ..
make 

运行
需要nginx的负载均衡
