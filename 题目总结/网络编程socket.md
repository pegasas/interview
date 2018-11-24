# 网络编程

## 惊群现象
惊群现象描述
Linux如何解决惊群的
Nginx如何解决惊群的
个人项目里面，如果要处理惊群，你要如何实现

bcopy和memcpy的区别？
事件驱动模型
字节序？（小端：低序字节存储在低地址；大端：低序字节存储在高地址）
TCP通信的各个系统调用
连接建立：socket、connect、bind、listen、accept、read、write（发送缓冲区）
连接关闭：close、shutdown（close与shutdown的区别？）
Linux服务器最大TCP连接数？（一个端口能接受tcp连接数量的理论上限？）
非阻塞connect？
连接建立过程中每个SYN可以包含哪些TCP选项？（MSS选项(TCP_MAXSEG)、窗口规模选项）作用是什么？
TCP连接建立过程中的超时
UDP通信的各个系统调用
socket、connect、bind、sendto（发送缓冲区）、recvfrom、close
连接UDP套接字与非连接UDP套接字的区别？性能？
数据发送：write、send、sendto、sendmsg
接收数据：read、recv、recvfrom、recvmsg
BIO与NIO、AIO
I/O复用：select、poll、epoll？
select：优缺点、中描述符集中最大描述符的个数（由FD_SETSIZE决定、怎么更改）？描述符就绪的条件？
poll：优缺点？事件？
epoll：优缺点、工作模式
Linux高性能服务器编程——进程池和线程池
100万并发连接服务器性能调优
14）TIME_WAIT和CLOSE_WAIT状态详解及性能调优
15）accept与epoll惊群
16）使用同步IO模型实现的Reactor模式的工作流程
IO多路复用
同步和异步，阻塞和非阻塞
select和epoll的区别
socket实现tcp,udp
