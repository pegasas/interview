# Dubbo底层并发通信原理

dubbo底层使用socket建立长连接，发送、接收数据的形式进行通信，结合使用apache mina框架，使用IoSession.write()方法，这个方法是一个异步的调用。
即对于当前线程来说，只需要将请求发送出去。就可以继续向后执行了。

基本原理
1、客户端一个线程调用远程接口，生成一个唯一的ID。dubbo使用AtomicLong从0开始计数。
2、将打包的方法调用信息（调用接口的接口名称、方法名称、参数值列表）以及处理结构的回调对象callback，全部封装在一起，组成一个对象Object
3、向专门存放调用信息的全局ConCurrentHashMap中put(ID,Obejct)
4、将ID和打包的方法调用信息封装成一对象connRequest，使用IoSession.write(connRequest)异步发送出去
5、**当前线程再使用callback的get()方法试图获取远程返回的结果。在get()内部使用synchronized获取回调对象的callback锁，再检测是否已经获得结果。如果没有，然后调用callback的wait()方法。
释放callback上的锁。让当前线程处于等待状态。**
6、服务端接收到请求并处理后。将结果发送给客户端。客户端上socket连接上专门监听消息的线程监听到消息。分析结果，取得ID,用ConCurrentHashMap.get(ID)。从而找到callback。将调用结果设置到callback对象中
7、监听线程接着使用synchronized获取回调对象的锁（callback的锁已经被释放掉了），再notifyAll(),唤醒前面处于等待状态的线程继续执行。

Q:当前线程如何”暂停”执行，等待结果回来后，再向后执行？

A:先生成一个对象Object，在全局的ConCurrentHashMap中存放起来，再用synchronized锁住这个对象，再调用wait()方法让当前线程处于等待状态。

另一消息监听线程等到服务端结果到来之后，再map.get(ID)找到Object.再用synchronized获取Object锁，再调用notifyAll()唤醒前面处于等待状态的线程。

从Dubbo开源文档中看到：Dubbo缺省协议采用单一长连接和NIO异步通讯，适合于小数据量大并发的服务调用，以及服务消费者机器数远大于服务提供者机器数的情况。
Dubbo通信默认采用的是Netty框架。Netty实质就是通过Socket进行通信，Socket(TCP)通信是全双工的方式。
因为采用单一长连接，所以如果消费者多线程请求，服务端处理完消息后返回，就会造成消息错乱的问题。解决这个问题的思路跟解决socket中的粘包问题类似。
socket粘包问题解决方法：用的最多的其实是定义一个定长的数据包头，其中包含了完整数据包的长度，以此来完成服务器端拆包工作。

类似的，那么dubbo解决上述问题的方法：就是给包头中添加一个全局唯一标识id，服务器端响应请求时也要携带这个id，供客户端多线程领取对应的响应数据提供线索。
