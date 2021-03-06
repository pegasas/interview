# Java对象JVM创建过程

1.去常量池检查是否存在new指令中包含的参数，比如new People(),则虚拟机首先会去常量池中检查是否有People这个类的符号引用，并且检查这个类是否已经被加载了，如果没有则会执行类加载过程。

2.为对象在java堆中分配内存，并且对象所需要分配的多大内存在类加载过程中就已经确定了。为对象分配内存的方式根据java堆是否规整分为两个方法：

- 指针碰撞：如果java堆是规整的，即所有用过的内存放在一边，没有用过的内存放在另外一边，并且有一个指针指向分界点，在需要为新生对象分配内存的时候，只需要移动指针画出一块内存分配和新生对象即可；

- 空闲列表：当java堆不是规整的，意思就是使用的内存和空闲内存交错在一起，这时候需要一张列表来记录哪些内存可使用，在需要为新生对象分配内存的时候，在这个列表中寻找一块大小合适的内存分配给它即可。而java堆是否规整和垃圾收集器是否带有压缩整理功能有关。

在为新生对象分配内存的时候，同时还需要考虑线程安全问题。因为在并发的情况下内存分配并不是线程安全的。
有两种方案解决这个线程安全问题，

- 为分配内存空间的动作进行同步处理；

- 为每个线程预先分配一小块内存，称为本地线程分配缓存（Thread Local Allocation Buffer, TLAB）,哪个线程需要分配内存，就在哪个线程的TLAB上分配。

3.虚拟机需要将每个对象分配到的内存初始化为0值（不包括对象头），这也就是为什么实例字段可以不用初始化，直接为0的原因。

4.虚拟机对对象进行必要的设置，例如这个对象属于哪个类的实例，如何找到类的元数据信息。对象的哈希吗、对象的GC年代等信息，这些信息都存放在对象头之中。

5.执行<init>指令，把对象按照程序员的指令进行初始化，这样一个对象就完整的创建出来。
