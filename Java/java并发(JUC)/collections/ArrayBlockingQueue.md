# ArrayBlockingQueue

ArrayBlockingQueue的原理就是使用一个可重入锁和这个锁生成的两个条件对象进行并发控制(classic two-condition algorithm)。

ArrayBlockingQueue是一个带有长度的阻塞队列，初始化的时候必须要指定队列长度，且指定长度之后不允许进行修改。

它带有的属性如下：

```
// 存储队列元素的数组，是个循环数组
final Object[] items;

// 拿数据的索引，用于take，poll，peek，remove方法
int takeIndex;

// 放数据的索引，用于put，offer，add方法
int putIndex;

// 元素个数
int count;

// 可重入锁
final ReentrantLock lock;
// notEmpty条件对象，由lock创建
private final Condition notEmpty;
// notFull条件对象，由lock创建
private final Condition notFull;
```

数据的添加

ArrayBlockingQueue有不同的几个数据添加方法，add、offer、put方法。

add方法：

```
public boolean add(E e) {
    if (offer(e))
        return true;
    else
        throw new IllegalStateException("Queue full");
}
```

add方法内部调用offer方法如下：

```
public boolean offer(E e) {
    checkNotNull(e); // 不允许元素为空
    final ReentrantLock lock = this.lock;
    lock.lock(); // 加锁，保证调用offer方法的时候只有1个线程
    try {
        if (count == items.length) // 如果队列已满
            return false; // 直接返回false，添加失败
        else {
            insert(e); // 数组没满的话调用insert方法
            return true; // 返回true，添加成功
        }
    } finally {
        lock.unlock(); // 释放锁，让其他线程可以调用offer方法
    }
}
```

insert方法如下：

```
private void insert(E x) {
    items[putIndex] = x; // 元素添加到数组里
    putIndex = inc(putIndex); // 放数据索引+1，当索引满了变成0
    ++count; // 元素个数+1
    notEmpty.signal(); // 使用条件对象notEmpty通知，比如使用take方法的时候队列里没有数据，被阻塞。这个时候队列insert了一条数据，需要调用signal进行通知
}
```

put方法：

```
public void put(E e) throws InterruptedException {
    checkNotNull(e); // 不允许元素为空
    final ReentrantLock lock = this.lock;
    lock.lockInterruptibly(); // 加锁，保证调用put方法的时候只有1个线程
    try {
        while (count == items.length) // 如果队列满了，阻塞当前线程，并加入到条件对象notFull的等待队列里
            notFull.await(); // 线程阻塞并被挂起，同时释放锁
        insert(e); // 调用insert方法
    } finally {
        lock.unlock(); // 释放锁，让其他线程可以调用put方法
    }
}
```

ArrayBlockingQueue的添加数据方法有add，put，offer这3个方法，总结如下：

add方法内部调用offer方法，如果队列满了，抛出IllegalStateException异常，否则返回true

offer方法如果队列满了，返回false，否则返回true

add方法和offer方法不会阻塞线程，put方法如果队列满了会阻塞线程，直到有线程消费了队列里的数据才有可能被唤醒。

这3个方法内部都会使用可重入锁保证原子性。

数据的删除

ArrayBlockingQueue有不同的几个数据删除方法，poll、take、remove方法。

poll方法：

```
public E poll() {
    final ReentrantLock lock = this.lock;
    lock.lock(); // 加锁，保证调用poll方法的时候只有1个线程
    try {
        return (count == 0) ? null : extract(); // 如果队列里没元素了，返回null，否则调用extract方法
    } finally {
        lock.unlock(); // 释放锁，让其他线程可以调用poll方法
    }
}
```

poll方法内部调用extract方法：

```
private E extract() {
    final Object[] items = this.items;
    E x = this.<E>cast(items[takeIndex]); // 得到取索引位置上的元素
    items[takeIndex] = null; // 对应取索引上的数据清空
    takeIndex = inc(takeIndex); // 取数据索引+1，当索引满了变成0
    --count; // 元素个数-1
    notFull.signal(); // 使用条件对象notFull通知，比如使用put方法放数据的时候队列已满，被阻塞。这个时候消费了一条数据，队列没满了，就需要调用signal进行通知
    return x; // 返回元素
}
```

take方法：

```
public E take() throws InterruptedException {
    final ReentrantLock lock = this.lock;
    lock.lockInterruptibly(); // 加锁，保证调用take方法的时候只有1个线程
    try {
        while (count == 0) // 如果队列空，阻塞当前线程，并加入到条件对象notEmpty的等待队列里
            notEmpty.await(); // 线程阻塞并被挂起，同时释放锁
        return extract(); // 调用extract方法
    } finally {
        lock.unlock(); // 释放锁，让其他线程可以调用take方法
    }
}
```

remove方法：

```
public boolean remove(Object o) {
    if (o == null) return false;
    final Object[] items = this.items;
    final ReentrantLock lock = this.lock;
    lock.lock(); // 加锁，保证调用remove方法的时候只有1个线程
    try {
        for (int i = takeIndex, k = count; k > 0; i = inc(i), k--) { // 遍历元素
            if (o.equals(items[i])) { // 两个对象相等的话
                removeAt(i); // 调用removeAt方法
                return true; // 删除成功，返回true
            }
        }
        return false; // 删除成功，返回false
    } finally {
        lock.unlock(); // 释放锁，让其他线程可以调用remove方法
    }
}
```

removeAt方法：

```
void removeAt(int i) {
    final Object[] items = this.items;
    if (i == takeIndex) { // 如果要删除数据的索引是取索引位置，直接删除取索引位置上的数据，然后取索引+1即可
        items[takeIndex] = null;
        takeIndex = inc(takeIndex);
    } else { // 如果要删除数据的索引不是取索引位置，移动元素元素，更新取索引和放索引的值
        for (;;) {
            int nexti = inc(i);
            if (nexti != putIndex) {
                items[i] = items[nexti];
                i = nexti;
            } else {
                items[i] = null;
                putIndex = i;
                break;
            }
        }
    }
    --count; // 元素个数-1
    notFull.signal(); // 使用条件对象notFull通知，比如使用put方法放数据的时候队列已满，被阻塞。这个时候消费了一条数据，队列没满了，就需要调用signal进行通知
}
```

ArrayBlockingQueue的删除数据方法有poll，take，remove这3个方法，总结如下：

poll方法对于队列为空的情况，返回null，否则返回队列头部元素。

remove方法取的元素是基于对象的下标值，删除成功返回true，否则返回false。

poll方法和remove方法不会阻塞线程。

take方法对于队列为空的情况，会阻塞并挂起当前线程，直到有数据加入到队列中。

这3个方法内部都会调用notFull.signal方法通知正在等待队列满情况下的阻塞线程。
