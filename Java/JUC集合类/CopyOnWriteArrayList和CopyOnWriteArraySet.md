# CopyOnWriteArrayList、CopyOnWriteArraySet

CopyOnWriteArrayList容器是Collections.synchronizedList(List list)的替代方案，CopyOnWriteArrayList在某些情况下具有更好的性能，考虑读远大于写的场景，如果把所有的读操作进行加锁，因为只有一个读线程能够获得锁，所以其他的读线程都必须等待，大大影响性能。CopyOnWriteArrayList称为“写时复制”容器，就是在多线程操作容器对象时，把容器复制一份，这样在线程内部的修改就与其他线程无关了，而且这样设计可以做到不阻塞其他的读线程。从JDK1.5开始Java并发包里提供了两个使用CopyOnWrite机制实现的并发容器，它们是CopyOnWriteArrayList和CopyOnWriteArraySet。

## CopyOnWriteArrayList源码

先说说CopyOnWriteArrayList容器的实现原理：简单地说，就是在需要对容器进行操作的时候，将容器拷贝一份，对容器的修改等操作都在容器的拷贝中进行，当操作结束，再把容器容器的拷贝指向原来的容器。这样设计的好处是实现了读写分离，并且读读不会发生阻塞。下面的源码是CopyOnWriteArrayList的add方法实现：

```
public void add(int index, E element) {
        final ReentrantLock lock = this.lock;
        lock.lock();
        try {
            Object[] elements = getArray();
            int len = elements.length;
            if (index > len || index < 0)
                throw new IndexOutOfBoundsException("Index: "+index+
                                                    ", Size: "+len);
            Object[] newElements;
            int numMoved = len - index;
            //1、复制出一个新的数组
            if (numMoved == 0)
                newElements = Arrays.copyOf(elements, len + 1);
            else {
                newElements = new Object[len + 1];
                System.arraycopy(elements, 0, newElements, 0, index);
                System.arraycopy(elements, index, newElements, index + 1,
                                 numMoved);
            }
            //2、把新元素添加到新数组中
            newElements[index] = element;
            //3、把数组指向原来的数组
            setArray(newElements);
        } finally {
            lock.unlock();
        }
    }
```

上面的三个步骤实现了写时复制的思想，在读数据的时候不会锁住list，因为写操作是在原来容器的拷贝上进行的。而且，可以看到，如果对容器拷贝修改的过程中又有新的读线程进来，那么读到的还是旧的数据。读的代码如下：

```
public E get(int index) {
        return get(getArray(), index);
    }

final Object[] getArray() {
       return array;
   }
```

CopyOnWrite并发容器用于读多写少的并发场景。比如白名单，黑名单，商品类目的访问和更新场景。

## CopyOnWriteArrayList的缺点

从CopyOnWriteArrayList的实现原理可以看到因为在需要容器对象的时候进行拷贝，所以存在两个问题：内存占用问题和数据一致性问题。

## 内存占用问题

因为需要将原来的对象进行拷贝，这需要一定的开销。特别是当容器对象过大的时候，因为拷贝而占用的内存将增加一倍（原来驻留在内存的对象仍然在使用，拷贝之后就有两份对象在内存中，所以增加了一倍内存）。而且，在高并发的场景下，因为每个线程都拷贝一份对象在内存中，这种情况体现得更明显。由于JVM的优化机制，将会触发频繁的Young GC和Full GC，从而使整个系统的性能下降。

## 数据一致性问题

CopyOnWriteArrayList不能保证实时一致性，因为读线程在将引用重新指向原来的对象之前再次读到的数据是旧的，所以CopyOnWriteArrayList 只能保证最终一致性。因此在需要实时一致性的场景CopyOnWriteArrayList是不能使用的。

## CopyOnWriteArrayList小结

CopyOnWriteArrayList适用于读多写少的场景
在并发操作容器对象时不会抛出ConcurrentModificationException，并且返回的元素与迭代器创建时的元素是一致的
容器对象的复制需要一定的开销，如果对象占用内存过大，可能造成频繁的YoungGC和Full GC
CopyOnWriteArrayList不能保证数据实时一致性，只能保证最终一致性

## CopyOnWriteArraySet

基于CopyOnWriteArrayList实现，其唯一的不同是在add时调用的是CopyOnWriteArrayList的addIfAbsent方法，其遍历当前Object数组，如Object数组中已有了当前元素，则直接返回，如果没有则放入Object数组的尾部，并返回。
