# 轻量级RPC框架

Linux环境下基于C++11实现的轻量级多线程的异步rpc框架：

- 框架采用**主从Reactor模式**，**主Reactor**线程负责监听连接套接字和处理客户端TCP连接，并将连接套接字分配至从属Reactor线程中。**从属Reactor**负责读取客户发送的数据，解码组装为RPC请求，执行RPC业务逻辑，最终将RPC响应返回给客户端。
- 主从Reactor底层采用**epoll实现IO多路复用**进而实现RPC异步调用，主Reactor eventloop事件负责监听连接套接字，从属Reactor eventloop事件数据可读和可写事件。
- 定时器使用**timerfd**进行实现，底层采用红黑树按照触发时间戳升序排列，当规定时间到达时便触发eventloop**可读事件**进而执行回调函数同时删除超时的任务。（服务端：定期删除已断开的客户，客户端：处理RPC调用超时）。
- 采用**阻塞队列**，实现**异步**日志系统记录服务器状态。线程将运行状态信息写入至队列中，间隔一定时间后由写日志线程将队列中的日志信息刷新至磁盘中。
- 为防止RPC串包，在应用层Protobuf 序列化协议的基础上，**自定义TinyPB协议**包含开始符，包长度，消息ID，方法名，错误信息，序列化数据和结束符。