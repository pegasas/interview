# java面向对象
java 多态，形式，应用
# Java类加载
Java反射 方法区
线程上下文类加载器

# Java GC


# JVM
## JVM GC
JVM永久区
为什么要用老年代和新生代？
新生代进入老生代的情况？（有三种）
提前担保机制？
新生代的分区？
怎样判断一个对象的存活周期长短(分代)
Minor GC和Full GC区别
什么时候会进行Full GC
g1回收器
hotspot
Java垃圾回收
java垃圾回收器
java内存溢出和解决
java栈溢出和解决
年轻代和老年代用的GC收集器
可达性算法，回收算法，Hotspot虚拟机回收机制，7种收集器。
## JVM内存模型
JVM内存划分
java中的堆和栈
堆和栈分别存什么

## JVM调优
JVM的调优参数

# Java内存模型

# java并发
1000个多并发线程，10台机器，每台机器4核的，设计线程池大小

# ThreadLocal

# ReentrantLock

# Java多线程
Thread,Runnable,Callable,Future(FutureTask)
## 线程状态
## 线程交互方式

# java锁
java锁种类
乐观锁如何保证线程安全

# Volatile
Volatile多线程
Volatile能保证原子性吗
内存屏障
volatile不线程安全
volatile实现原理，内存屏障

# Synchronized
Synchronized、Lock、reentrantlock
synchronized的实现原理
synchronize对static方法加锁
可重入锁，synchronize是否可重入
synchronized在方法上是对象锁还是类锁
synchronized关键字底层什么命令
synchronized为什么是非公平的

# Java线程池
线程池参数，实现原理
线程池有哪些参数
多线程例子
线程池有哪些RejectedExecutionHandler,分别对应的使用场景
juc的线程池

# Java atomic

# java集合类
Blockingqueue
## ArrayList、LinkedList
ArrayList和LinkedList底层
## HashMap（HashSet） ConcurrentHashMap
HashMap场景
HashMap原理（HashSet原理）
ConcurrentHashMap原理
ConcurrentHashMap使用场景及原因（多线程查询数据，put可能覆盖，或者hashmap的扩容可能产生死链）
HashMap快速失败（fast-Fail）机制
HashMap get()实现
HashMap put()实现
HashMap和ConcurrentHashMap jdk1.7 jdk1.8
concurrenthashmap和hashmap的区别
JDK1.8 ConcurrentHashMap分段锁的实现和优化
hashmap初始容量必须2的次幂原因

## HashTable
hashtable原理
hashmap可以存null值，hashtable不可以

## TreeMap（TreeSet）
TreeMap的底层原理
hashmap和hashtable区别











# Java并发

# CountDownLatch
# CyclicBarrier
# Semaphore

# static
static方法不可以访问非static方法或变量

# final
final修饰的对象可以改变值，不能改变应用
匿名内部类访问外部变量(final)


# Spring，SpringMVC，Mybatis，Hibernate，Struts2
Spring，SpringMVC，Mybatis，Hibernate，Struts2
## Spring
Spring优点
### Spring IOC
Spring IOC实现
Spring IOC容器的本质
### Spring AOP
Spring AOP实现
Spring AOP加一层动态代理（创建bean过程中直接生成代理类的实例）

Spring拦截器过滤器
SpringBoot作用
SpringBoot和SpringCloud
Spring的IoC和DI
spring当中事物的隔离级别
spring用到的设计模式
AOP用到的设计模式
spring容器的启动过程
cglib无法增强的类
动态代理的实现原理
Spring的bean的作用域（singleton，prototype）
Spring的IOC实现原理？没有无参构造函数能实例化吗？有参构造函数注入？（xml配置）
Spring AOP 实现原理, 大致讲一下, 反射里面具体的类是什么
Spring Bean加载流程
Spring中xml配置Bean和注解配置区别, 顺序，怎么解析它们的数据
Spring的spring quartz
SpringMVC5大组件
Spring如何动态地加载一个bean到bean容器中，不通过配置文件配置的
## SpringMVC
Spring MVC的请求过程
spring mvc的原理和请求过程
## Mybatis
Mybatis的作用
## Hibernate
hibernate跟mybatis异同
Jdbc要查询一条语句讲一下流程？用完JDBC关闭流程介绍一下？
hibernate的orm是怎么实现的，我说的是xml和注解的解析，以及jdbc封装
hibernate的事务如何实现，我说是jdbc的事务实现。
hibernate的注解可以实现切换。
## Struts2
Mapping映射和Mybatis与Struts2的大致区别



# java BIO/NIO/AIO
nio与bio的区别。
nio的实现（没问原理，问原理估计要扯到linux的io模型了）
## netty
nio的包装框架netty(从rpc和微服务的角度说了一下。问我有哪些序列化方式。)
nio和io的区别，nio是阻塞轮询的，如何改用异步通知的机制，我说使用aio注册异步回调函数。

# 基础
Lambda表达式
java分布式缓存框架
java如何做到平台无关
==和equels的区别

# java object
object类equals方法是怎么实现

java 静态代码块与构造方法谁先初始化，答静态代码块，原因
hashcode怎么重写
重写equels需要重写hashcode吗，为什么？
静态块只执行一次原因
为什么重写hashcode
JDK动态代理
jdk1.7和1.8的区别
接口的默认方法和静态方法
Java的深拷贝和浅拷贝
blockingqueue
不使用sleep，如何达到进程睡眠的效果
java 重写和重载的区别
java 可变参数
java 泛型(jdk源码层面)
# java线程安全
保证线程安全的方式，静态变量、局部变量线程安全

java内存泄漏，导致问题，内存泄漏防止

# java异常
java异常
