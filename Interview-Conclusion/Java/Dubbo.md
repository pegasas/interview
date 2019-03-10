### 1.1 Dubbo

#### 1.1.1 Dubbo架构

![avatar][Dubbo架构]

Provider: 暴露服务的服务提供方。
Consumer: 调用远程服务的服务消费方。
Registry: 服务注册与发现的注册中心。
Monitor: 统计服务的调用次调和调用时间的监控中心。
Container: 服务运行容器。

服务容器负责启动，加载，运行服务提供者
服务提供者在启动时，向注册中心注册自己提供的服务。
服务消费者在启动时，向注册中心订阅自己所需的服务。
注册中心返回服务提供者地址列表给消费者，如果有变更，注册中心将基于长连接推送变更数据给消费者。
服务消费者，从提供者地址列表中，基于软负载均衡算法，选一台提供者进行调用，如果调用失败，再选另一台调用。
服务消费者和提供者，在内存中累计调用次数和调用时间，定时每分钟发送一次统计数据到监控中心。

### 1.1.2 Dubbo通信协议

dubbo://    Dubbo缺省协议采用单一长连接和NIO异步通讯，适合于小数据量大并发的服务调用，以及服务消费者机器数远大于服务提供者机器数的情况。
rmi://  RMI协议采用JDK标准的java.rmi.实现，采用阻塞式短连接和JDK标准序列化方式*
hessian://
http://
webservice://
thrift://
memcached://
redis://

### 1.1.3 Dubbo服务调用超时解决

1.对于核心的服务中心，去除dubbo超时重试机制，并重新评估设置超时时间。
2.业务处理代码必须放在服务端，客户端只做参数验证和服务调用，不涉及业务流程处理

### 1.1.4 Dubbo注册中心

Multicast注册中心
Zookeeper注册中心
Redis注册中心
Simple注册中心

### 1.1.5 Dubbo服务容器

Spring Container
Jetty Container
Log4j Container

Dubbo 的服务容器只是一个简单的 Main 方法，并加载一个简单的 Spring 容器，用于暴露服务。

### 1.1.6 Dubbo序列化

Hessian序列化
Duddo
FastJson
Java自带序列化
