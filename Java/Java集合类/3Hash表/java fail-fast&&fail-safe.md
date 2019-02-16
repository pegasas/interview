# java fail-fast&&fail-safe

fail-fast机制在遍历一个集合时，当集合结构被修改，
会抛出Concurrent Modification Exception。

fail-fast会在以下两种情况下抛出ConcurrentModificationException

（1）单线程环境

集合被创建后，在遍历它的过程中修改了结构。

注意 remove()方法会让expectModcount和modcount 相等，所以是不会抛出这个异常。

（2）多线程环境

当一个线程在遍历这个集合，而另一个线程对这个集合的结构进行了修改。
注意，迭代器的快速失败行为无法得到保证，因为一般来说，
不可能对是否出现不同步并发修改做出任何硬性保证。快速失败迭代器会尽最大努力抛出
ConcurrentModificationException。因此，为提高这类迭代器的正确性而编写一个
依赖于此异常的程序是错误的做法：迭代器的快速失败行为应该仅用于检测 bug。

3. fail-fast机制是如何检测的？

迭代器在遍历过程中是直接访问内部数据的，因此内部的数据在遍历的过程中无法被修改。
为了保证不被修改，迭代器内部维护了一个标记 “mode” ，当集合结构改变
（添加删除或者修改），标记"mode"会被修改，而迭代器每次的hasNext()和next()方法
都会检查该"mode"是否被改变，当检测到被修改时，抛出Concurrent Modification
Exception

4. fail-safe机制

fail-safe任何对集合结构的修改都会在一个复制的集合上进行修改，
因此不会抛出ConcurrentModificationException

fail-safe机制有两个问题

（1）需要复制集合，产生大量的无效对象，开销大

（2）无法保证读取的数据是目前原始数据结构中的数据。
