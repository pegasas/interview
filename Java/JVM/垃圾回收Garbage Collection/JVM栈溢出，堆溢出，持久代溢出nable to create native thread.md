# Java JVM：内存溢出（栈溢出，堆溢出，持久代溢出以及 nable to create native thread）

包括：
1. 栈溢出(StackOverflowError)
2. 堆溢出(OutOfMemoryError:java heap space)
3. 永久代溢出(OutOfMemoryError: PermGen space)
4. OutOfMemoryError:unable to create native thread

Java虚拟机规范规定JVM的内存分为了好几块，比如堆，栈，程序计数器，方法区等，而Hotspot jvm的实现中，将堆内存分为了两部：新生代，老年代。在堆内存之外，还有永久代，其中永久代实现了规范中规定的方法区，而内存模型中不同的部分都会出现相应的OOM错误，接下来我们就分开来讨论一下。
