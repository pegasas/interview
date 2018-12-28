# ReentrantLock的实现

## lock()

ReentrantLock中有一个抽象类Sync：
```
private final Sync sync;

/**
 * Base of synchronization control for this lock. Subclassed
 * into fair and nonfair versions below. Uses AQS state to
 * represent the number of holds on the lock.
 */
abstract static class Sync extends AbstractQueuedSynchronizer {
...
}
```

ReentrantLock根据传入构造方法的布尔型参数实例化出Sync的实现类FairSync和NonfairSync，分别表示公平的Sync和非公平的Sync。由于ReentrantLock我们用的比较多的是非公平锁，所以看下非公平锁是如何实现的。假设线程1调用了ReentrantLock的lock()方法，那么线程1将会独占锁，整个调用链十分简单：

![avatar][线程1的方法调用链]

第一个获取锁的线程就做了两件事情：

1、设置AbstractQueuedSynchronizer的state为1

2、设置AbstractOwnableSynchronizer的thread为当前线程

这两步做完之后就表示线程1独占了锁。然后线程2也要尝试获取同一个锁，在线程1没有释放锁的情况下必然是行不通的，所以线程2就要阻塞。那么，线程2如何被阻塞？看下线程2的方法调用链，这就比较复杂了：

![avatar][线程2的方法调用链]

调用链看到确实非常长，没关系，结合代码分析一下，其实ReentrantLock没有那么复杂，我们一点点来扒代码：

```
final void lock() {
    if (compareAndSetState(0, 1))
         setExclusiveOwnerThread(Thread.currentThread());
    else
         acquire(1);
}
```

首先线程2尝试利用CAS去判断state是不是0，是0就设置为1，当然这一步操作肯定是失败的，因为线程1已经将state设置成了1，所以第2行必定是false，因此线程2走第5行的acquire方法：

```
public final void acquire(int arg) {
    if (!tryAcquire(arg) &&
        acquireQueued(addWaiter(Node.EXCLUSIVE), arg))
        selfInterrupt();
}
```

从字面上就很好理解这个if的意思，先走第一个判断条件尝试获取一次锁，如果获取的结果为false即失败，走第二个判断条件添加FIFO等待队列。所以先看一下tryAcquire方法做了什么，这个方法最终调用到的是Sync的nonfairTryAcquire方法：

```
final boolean nonfairTryAcquire(int acquires) {
    final Thread current = Thread.currentThread();
    int c = getState();
    if (c == 0) {
        if (compareAndSetState(0, acquires)) {
            setExclusiveOwnerThread(current);
            return true;
        }
    }
    else if (current == getExclusiveOwnerThread()) {
        int nextc = c + acquires;
        if (nextc < 0) // overflow
            throw new Error("Maximum lock count exceeded");
        setState(nextc);
        return true;
    }
    return false;
}
```

由于state是volatile的，所以state对线程2具有可见性，线程2拿到最新的state，再次判断一下能否持有锁（可能线程1同步代码执行得比较快，这会儿已经释放了锁），不可以就返回false。

注意一下第10~第16行，这段代码的作用是让某个线程可以多次调用同一个ReentrantLock，每调用一次给state+1，由于某个线程已经持有了锁，所以这里不会有竞争，因此不需要利用CAS设置state（相当于一个偏向锁）。从这段代码可以看到，nextc每次加1，当nextc<0的时候抛出error，那么同一个锁最多能重入Integer.MAX_VALUE次，也就是2147483647。

然后就走到if的第二个判断里面了，先走AQS的addWaiter方法：

```
private Node addWaiter(Node mode) {
    Node node = new Node(Thread.currentThread(), mode);
    // Try the fast path of enq; backup to full enq on failure
    Node pred = tail;
    if (pred != null) {
        node.prev = pred;
        if (compareAndSetTail(pred, node)) {
            pred.next = node;
            return node;
        }
    }
    enq(node);
    return node;
}
```

先创建一个当前线程的Node，模式为独占模式（因为传入的mode是一个NULL），再判断一下队列上有没有节点，没有就创建一个队列，因此走enq方法：

```
private Node enq(final Node node) {
    for (;;) {
        Node t = tail;
        if (t == null) { // Must initialize
            Node h = new Node(); // Dummy header
            h.next = node;
            node.prev = h;
            if (compareAndSetHead(h)) {
                tail = node;
                return h;
            }
        }
        else {
            node.prev = t;
            if (compareAndSetTail(t, node)) {
                t.next = node;
                return t;
            }
        }
    }
}
```

这个方法其实画一张图应该比较好理解，形成一个队列之后应该是这样的：

![avatar][if的第二个判断]

每一步都用图表示出来了，由于线程2所在的Node是第一个要等待的Node，因此FIFO队列上肯定没有内容，tail为null，走的就是第4行~第10行的代码逻辑。这里用了CAS设置头Node，当然有可能线程2设置头Node的时候CPU切换了，线程3已经把头Node设置好了形成了上图所示的一个队列，这时线程2再循环一次获取tail，由于tail是volatile的，所以对线程2可见，线程2看见tail不为null，就走到了13行的else里面去往尾Node后面添加自身。整个过程下来，形成了一个双向队列。最后走AQS的acquireQueued(node, 1)：

```
final boolean acquireQueued(final Node node, int arg) {
    try {
        boolean interrupted = false;
        for (;;) {
            final Node p = node.predecessor();
            if (p == head && tryAcquire(arg)) {
                setHead(node);
                p.next = null; // help GC
                return interrupted;
            }
            if (shouldParkAfterFailedAcquire(p, node) &&
                parkAndCheckInterrupt())
                interrupted = true;
        }
    } catch (RuntimeException ex) {
        cancelAcquire(node);
        throw ex;
    }
}
```

此时再做判断，由于线程2是双向队列的真正的第一个Node（前面还有一个h），所以第5行~第10行再次判断一下线程2能不能获取锁（可能这段时间内线程1已经执行完了把锁释放了，state从1变为了0），如果还是不行，先调用AQS的shouldParkAfterFailedAcquire(p, node)方法：

```
private static boolean shouldParkAfterFailedAcquire(Node pred, Node node) {
    int s = pred.waitStatus;
    if (s < 0)
        /*
         * This node has already set status asking a release
         * to signal it, so it can safely park
         */
        return true;
    if (s > 0) {
        /*
         * Predecessor was cancelled. Skip over predecessors and
         * indicate retry.
         */
    do {
    node.prev = pred = pred.prev;
    } while (pred.waitStatus > 0);
    pred.next = node;
}
    else
        /*
         * Indicate that we need a signal, but don't park yet. Caller
         * will need to retry to make sure it cannot acquire before
         * parking.
         */
         compareAndSetWaitStatus(pred, 0, Node.SIGNAL);
    return false;
}
```

h的waitStatus，很明显是0，所以此时把h的waitStatus设置为Noed.SIGNAL即-1并返回false。既然返回了false，上面的acquireQueued的11行if自然不成立，再走一次for循环，还是先尝试获取锁，不成功，继续走shouldParkAfterFailedAcquire，此时waitStatus为-1，小于0，走第三行的判断，返回true。然后走acquireQueued的11行if的第二个判断条件parkAndCheckInterrupt：

```
private final boolean parkAndCheckInterrupt() {
    LockSupport.park(this);
    return Thread.interrupted();
}
public static void park(Object blocker) {
    Thread t = Thread.currentThread();
    setBlocker(t, blocker);
    unsafe.park(false, 0L);
    setBlocker(t, null);
}
```

最后一步，调用LockSupport的park方法阻塞住了当前的线程。至此，使用ReentrantLock让线程1独占锁、线程2进入FIFO队列并阻塞的完整流程已经整理出来了。

lock()的操作明了之后，就要探究一下unlock()的时候代码又做了什么了，接着看下一部分。

## unlock()

```
public void unlock() {
    sync.release(1);
}
```

走AQS的release：

```
public final boolean release(int arg) {
    if (tryRelease(arg)) {
        Node h = head;
        if (h != null && h.waitStatus != 0)
           unparkSuccessor(h);
        return true;
    }
    return false;
}
```

先调用Sync的tryRelease尝试释放锁：

```
protected final boolean tryRelease(int releases) {
    int c = getState() - releases;
    if (Thread.currentThread() != getExclusiveOwnerThread())
        throw new IllegalMonitorStateException();
    boolean free = false;
    if (c == 0) {
        free = true;
        setExclusiveOwnerThread(null);
    }
    setState(c);
    return free;
}
```

首先，只有当c==0的时候才会让free=true，这和上面一个线程多次调用lock方法累加state是对应的，调用了多少次的lock()方法自然必须调用同样次数的unlock()方法才行，这样才把一个锁给全部解开。

当一条线程对同一个ReentrantLock全部解锁之后，AQS的state自然就是0了，AbstractOwnableSynchronizer的exclusiveOwnerThread将被设置为null，这样就表示没有线程占有锁，方法返回true。代码继续往下走，上面的release方法的第四行，h不为null成立，h的waitStatus为-1，不等于0也成立，所以走第5行的unparkSuccessor方法：

```
private void unparkSuccessor(Node node) {
    /*
     * Try to clear status in anticipation of signalling.  It is
     * OK if this fails or if status is changed by waiting thread.
     */
    compareAndSetWaitStatus(node, Node.SIGNAL, 0);

    /*
     * Thread to unpark is held in successor, which is normally
     * just the next node.  But if cancelled or apparently null,
     * traverse backwards from tail to find the actual
     * non-cancelled successor.
     */
    Node s = node.next;
    if (s == null || s.waitStatus > 0) {
        s = null;
        for (Node t = tail; t != null && t != node; t = t.prev)
            if (t.waitStatus <= 0)
                s = t;
   }
    if (s != null)
        LockSupport.unpark(s.thread);
}
```

s即h的下一个Node，这个Node里面的线程就是线程2，由于这个Node不等于null，所以走21行，线程2被unPark了，得以运行。有一个很重要的问题是：锁被解了怎样保证整个FIFO队列减少一个Node呢？这是一个很巧妙的设计，又回到了AQS的acquireQueued方法了：

```
final boolean acquireQueued(final Node node, int arg) {
    try {
        boolean interrupted = false;
        for (;;) {
            final Node p = node.predecessor();
            if (p == head && tryAcquire(arg)) {
                setHead(node);
                p.next = null; // help GC
                return interrupted;
            }
            if (shouldParkAfterFailedAcquire(p, node) &&
                parkAndCheckInterrupt())
                interrupted = true;
        }
    } catch (RuntimeException ex) {
        cancelAcquire(node);
        throw ex;
    }
}
```

被阻塞的线程2是被阻塞在第12行，注意这里并没有return语句，也就是说，阻塞完成线程2依然会进行for循环。然后，阻塞完成了，线程2所在的Node的前驱Node是p，线程2尝试tryAcquire，成功，然后线程2就成为了head节点了，把p的next设置为null，这样原头Node里面的所有对象都不指向任何块内存空间，h属于栈内存的内容，方法结束被自动回收，这样随着方法的调用完毕，原头Node也没有任何的引用指向它了，这样它就被GC自动回收了。此时，遇到一个return语句，acquireQueued方法结束，后面的Node也是一样的原理。

这里有一个细节，看一下setHead方法：

```
private void setHead(Node node) {
    head = node;
    node.thread = null;
    node.prev = null;
}
```

setHead方法里面的前驱Node是Null，也没有线程，那么为什么不用一个在等待的线程作为Head Node呢？

因为一个线程随时有可能因为中断而取消，而取消的话，Node自然就要被GC了，那GC前必然要把头Node的后继Node变为一个新的头而且要应对多种情况，这样就很麻烦。用一个没有thread的Node作为头，相当于起了一个引导作用，因为head没有线程，自然也不会被取消。

再看一下上面unparkSuccessor的14行~20行，就是为了防止head的下一个node被取消的情况，这样，就从尾到头遍历，找出离head最近的一个node，对这个node进行unPark操作。

[if的第二个判断]:data:image/png;base64,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

[线程2的方法调用链]:data:image/png;base64,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

[线程1的方法调用链]:data:image/png;base64,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
