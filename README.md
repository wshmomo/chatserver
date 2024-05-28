# chatserver
可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端源码 基于muduo实习

1.编译方式

cd build

rm -rf *

cmake ..

make 

2.运行
2.1在bin目录下运行，服务器 ./ChatServer 127.0.0.1 6000  //这里需要给服务器分配ip地址和端口号。这里为了加大并发量, nginx tcp负载均衡可以分配给两台服务器
第二台服务器 ./ChatServer 127.0.0.1 6002

2.2客户端与nginx tcp负载均衡相连，故客户端 ./ChatClient 127.0.0.1 8000  //这里需要给nginx.conf配置tcp负载均衡，这里的端口设为8000

