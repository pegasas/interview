# ArrayQueue

Queue 也是 Java 集合框架中定义的一种接口，直接继承自 Collection 接口。除了基本的 Collection 接口规定测操作外，Queue 接口还定义一组针对队列的特殊操作。通常来说，Queue 是按照先进先出(FIFO)的方式来管理其中的元素的，但是优先队列是一个例外。

Deque 接口继承自 Queue接口，但 Deque 支持同时从两端添加或移除元素，因此又被成为双端队列。鉴于此，Deque 接口的实现可以被当作 FIFO队列使用，也可以当作LIFO队列（栈）来使用。官方也是推荐使用 Deque 的实现来替代 Stack。

ArrayDeque 是 Deque 接口的一种具体实现，是依赖于可变数组来实现的。ArrayDeque 没有容量限制，可根据需求自动进行扩容。ArrayDeque不支持值为 null 的元素。

```
public interface Queue<E> extends Collection<E> {
    //向队列中插入一个元素，并返回true
    //如果队列已满，抛出IllegalStateException异常
    boolean add(E e);

    //向队列中插入一个元素，并返回true
    //如果队列已满，返回false
    boolean offer(E e);

    //取出队列头部的元素，并从队列中移除
    //队列为空，抛出NoSuchElementException异常
    E remove();

    //取出队列头部的元素，并从队列中移除
    //队列为空，返回null
    E poll();

    //取出队列头部的元素，但并不移除
    //如果队列为空，抛出NoSuchElementException异常
    E element();

    //取出队列头部的元素，但并不移除
    //队列为空，返回null
    E peek();
}
```

Deque 提供了双端的插入与移除操作

底层结构

```
//用数组存储元素
transient Object[] elements; // non-private to simplify nested class access
//头部元素的索引
transient int head;
//尾部下一个将要被加入的元素的索引
transient int tail;
//最小容量，必须为2的幂次方
private static final int MIN_INITIAL_CAPACITY = 8;
```

在 ArrayDeque 底部是使用数组存储元素，同时还使用了两个索引来表征当前数组的状态，分别是 head 和 tail。head 是头部元素的索引，但注意 tail 不是尾部元素的索引，而是尾部元素的下一位，即下一个将要被加入的元素的索引。

初始化
ArrayDeque 提供了三个构造方法，分别是默认容量，指定容量及依据给定的集合中的元素进行创建。默认容量为16。

```
public ArrayDeque() {
    elements = new Object[16];
}

public ArrayDeque(int numElements) {
    allocateElements(numElements);
}

public ArrayDeque(Collection<? extends E> c) {
    allocateElements(c.size());
    addAll(c);
}
```

ArrayDeque 对数组的大小(即队列的容量)有特殊的要求，必须是 2^n。通过 allocateElements方法计算初始容量：

```
private void allocateElements(int numElements) {
    int initialCapacity = MIN_INITIAL_CAPACITY;
    // Find the best power of two to hold elements.
    // Tests "<=" because arrays aren't kept full.
    if (numElements >= initialCapacity) {
        initialCapacity = numElements;
        initialCapacity |= (initialCapacity >>>  1);
        initialCapacity |= (initialCapacity >>>  2);
        initialCapacity |= (initialCapacity >>>  4);
        initialCapacity |= (initialCapacity >>>  8);
        initialCapacity |= (initialCapacity >>> 16);
        initialCapacity++;

        if (initialCapacity < 0)   // Too many elements, must back off
            initialCapacity >>>= 1;// Good luck allocating 2 ^ 30 elements
    }
    elements = new Object[initialCapacity];
}
```

>>>是无符号右移操作，|是位或操作，经过五次右移和位或操作可以保证得到大小为2^k-1的数。

添加元素
向末尾添加元素：

```
public void addLast(E e) {
        if (e == null)
            throw new NullPointerException();
        //tail 中保存的是即将加入末尾的元素的索引
        elements[tail] = e;
        //tail 向后移动一位
        //把数组当作环形的，越界后到0索引
        if ( (tail = (tail + 1) & (elements.length - 1)) == head)
            //tail 和 head相遇，空间用尽，需要扩容
            doubleCapacity();
    }
```

这段代码中，(tail = (tail + 1) & (elements.length - 1)) == head这句有点难以理解。其实，在 ArrayDeque 中数组是当作环形来使用的，索引0看作是紧挨着索引(length-1)之后的。参考下面的图片：

![avatar][ArrayDeque数组环形]

在容量保证为 2^n 的情况下，仅仅通过位与操作就可以完成环形索引的计算，而不需要进行边界的判断，在实现上更为高效。

向头部添加元素的代码如下：

```
public void addFirst(E e) {
    if (e == null) //不支持值为null的元素
        throw new NullPointerException();
    elements[head = (head - 1) & (elements.length - 1)] = e;
    if (head == tail)
        doubleCapacity();
}
```

其它的诸如add，offer，offerFirst，offerLast等方法都是基于上面这两个方法实现的

扩容
在每次添加元素后，如果头索引和尾部索引相遇，则说明数组空间已满，需要进行扩容操作。 ArrayDeque 每次扩容都会在原有的容量上翻倍，这也是对容量必须是2的幂次方的保证。

```
private void doubleCapacity() {
    assert head == tail; //扩容时头部索引和尾部索引肯定相等
    int p = head;
    int n = elements.length;
    //头部索引到数组末端(length-1处)共有多少元素
    int r = n - p; // number of elements to the right of p
    //容量翻倍
    int newCapacity = n << 1;
    //容量过大，溢出了
    if (newCapacity < 0)
        throw new IllegalStateException("Sorry, deque too big");
    //分配新空间
    Object[] a = new Object[newCapacity];
    //复制头部索引到数组末端的元素到新数组的头部
    System.arraycopy(elements, p, a, 0, r);
    //复制其余元素
    System.arraycopy(elements, 0, a, r, p);
    elements = a;
    //重置头尾索引
    head = 0;
    tail = n;
}
```

移除元素
ArrayDeque支持从头尾两端移除元素，remove方法是通过poll来实现的。因为是基于数组的，在了解了环的原理后这段代码就比较容易理解了。

```
public E pollFirst() {
    int h = head;
    @SuppressWarnings("unchecked")
    E result = (E) elements[h];
    // Element is null if deque empty
    if (result == null)
        return null;
    elements[h] = null;     // Must null out slot
    head = (h + 1) & (elements.length - 1);
    return result;
}

public E pollLast() {
    int t = (tail - 1) & (elements.length - 1);
    @SuppressWarnings("unchecked")
    E result = (E) elements[t];
    if (result == null)
        return null;
    elements[t] = null;
    tail = t;
    return result;
}
```

获取队头和队尾的元素

```
@SuppressWarnings("unchecked")
public E peekFirst() {
    // elements[head] is null if deque empty
    return (E) elements[head];
}

@SuppressWarnings("unchecked")
public E peekLast() {
    return (E) elements[(tail - 1) & (elements.length - 1)];
}
```

迭代器
ArrayDeque 在迭代是检查并发修改并没有使用类似于 ArrayList 等容器中使用的 modCount，而是通过尾部索引的来确定的。具体参考 next 方法中的注释。但是这样不一定能保证检测到所有的并发修改情况，加入先移除了尾部元素，又添加了一个尾部元素，这种情况下迭代器是没法检测出来的。

```
private class DeqIterator implements Iterator<E> {
    /**
     * Index of element to be returned by subsequent call to next.
     */
    private int cursor = head;

    /**
     * Tail recorded at construction (also in remove), to stop
     * iterator and also to check for comodification.
     */
    private int fence = tail;

    /**
     * Index of element returned by most recent call to next.
     * Reset to -1 if element is deleted by a call to remove.
     */
    private int lastRet = -1;

    public boolean hasNext() {
        return cursor != fence;
    }

    public E next() {
        if (cursor == fence)
            throw new NoSuchElementException();
        @SuppressWarnings("unchecked")
        E result = (E) elements[cursor];
        // This check doesn't catch all possible comodifications,
        // but does catch the ones that corrupt traversal
        // 如果移除了尾部元素，会导致tail != fence
        // 如果移除了头部元素，会导致 result == null
        if (tail != fence || result == null)
            throw new ConcurrentModificationException();
        lastRet = cursor;
        cursor = (cursor + 1) & (elements.length - 1);
        return result;
    }

    public void remove() {
        if (lastRet < 0)
            throw new IllegalStateException();
        if (delete(lastRet)) { // if left-shifted, undo increment in next()
            cursor = (cursor - 1) & (elements.length - 1);
            fence = tail;
        }
        lastRet = -1;
    }

    public void forEachRemaining(Consumer<? super E> action) {
        Objects.requireNonNull(action);
        Object[] a = elements;
        int m = a.length - 1, f = fence, i = cursor;
        cursor = f;
        while (i != f) {
            @SuppressWarnings("unchecked") E e = (E)a[i];
            i = (i + 1) & m;
            if (e == null)
                throw new ConcurrentModificationException();
            action.accept(e);
        }
    }
}
```

除了 DeqIterator，还有一个反向的迭代器 DescendingIterator，顺序和 DeqIterator 相反。

小结
ArrayDeque 是 Deque 接口的一种具体实现，是依赖于可变数组来实现的。ArrayDeque 没有容量限制，可根据需求自动进行扩容。ArrayDeque 可以作为栈来使用，效率要高于 Stack；ArrayDeque 也可以作为队列来使用，效率相较于基于双向链表的 LinkedList 也要更好一些。注意，ArrayDeque 不支持为 null 的元素。

[ArrayDeque数组环形]:data:image/png;base64,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
