# 计数信号量Semaphore

Semaphore简介

Semaphore是一个计数信号量，它的本质是一个”共享锁”。

信号量维护了一个信号量许可集。线程可以通过调用acquire()来获取信号量的许可；当信号量中有可用的许可时，线程能获取该许可；否则线程必须等待，直到有可用的许可为止。 线程可以通过release()来释放它所持有的信号量许可。

Semaphore的函数列表

```
// 创建具有给定的许可数和非公平的公平设置的 Semaphore。
Semaphore(int permits)
// 创建具有给定的许可数和给定的公平设置的 Semaphore。
Semaphore(int permits, boolean fair)

// 从此信号量获取一个许可，在提供一个许可前一直将线程阻塞，否则线程被中断。
void acquire()
// 从此信号量获取给定数目的许可，在提供这些许可前一直将线程阻塞，或者线程已被中断。
void acquire(int permits)
// 从此信号量中获取许可，在有可用的许可前将其阻塞。
void acquireUninterruptibly()
// 从此信号量获取给定数目的许可，在提供这些许可前一直将线程阻塞。
void acquireUninterruptibly(int permits)
// 返回此信号量中当前可用的许可数。
int availablePermits()
// 获取并返回立即可用的所有许可。
int drainPermits()
// 返回一个 collection，包含可能等待获取的线程。
protected Collection<Thread> getQueuedThreads()
// 返回正在等待获取的线程的估计数目。
int getQueueLength()
// 查询是否有线程正在等待获取。
boolean hasQueuedThreads()
// 如果此信号量的公平设置为 true，则返回 true。
boolean isFair()
// 根据指定的缩减量减小可用许可的数目。
protected void reducePermits(int reduction)
// 释放一个许可，将其返回给信号量。
void release()
// 释放给定数目的许可，将其返回到信号量。
void release(int permits)
// 返回标识此信号量的字符串，以及信号量的状态。
String toString()
// 仅在调用时此信号量存在一个可用许可，才从信号量获取许可。
boolean tryAcquire()
// 仅在调用时此信号量中有给定数目的许可时，才从此信号量中获取这些许可。
boolean tryAcquire(int permits)
// 如果在给定的等待时间内此信号量有可用的所有许可，并且当前线程未被中断，则从此信号量获取给定数目的许可。
boolean tryAcquire(int permits, long timeout, TimeUnit unit)
// 如果在给定的等待时间内，此信号量有可用的许可并且当前线程未被中断，则从此信号量获取一个许可。
boolean tryAcquire(long timeout, TimeUnit unit)
```

Semaphore数据结构

Semaphore的UML类图如下：
![avatar][1]

(01) 和”ReentrantLock”一样，Semaphore也包含了sync对象，sync是Sync类型；而且，Sync是一个继承于AQS的抽象类。
(02) Sync包括两个子类：”公平信号量”FairSync 和 “非公平信号量”NonfairSync。sync是”FairSync的实例”，或者”NonfairSync的实例”；默认情况下，sync是NonfairSync(即，默认是非公平信号量)。

Semaphore源码分析(基于JDK1.7.0_40)

Semaphore是通过共享锁实现的。根据共享锁的获取原则，Semaphore分为”公平信号量”和”非公平信号量”。

“公平信号量”和”非公平信号量”的区别

“公平信号量”和”非公平信号量”的释放信号量的机制是一样的！不同的是它们获取信号量的机制：线程在尝试获取信号量许可时，对于公平信号量而言，如果当前线程不在CLH队列的头部，则排队等候；而对于非公平信号量而言，无论当前线程是不是在CLH队列的头部，它都会直接获取信号量。该差异具体的体现在，它们的tryAcquireShared()函数的实现不同。

“公平信号量”类

```
static final class FairSync extends Sync {
    private static final long serialVersionUID = 2014338818796000944L;

    FairSync(int permits) {
        super(permits);
    }

    protected int tryAcquireShared(int acquires) {
        for (;;) {
            if (hasQueuedPredecessors())
                return -1;
            int available = getState();
            int remaining = available - acquires;
            if (remaining < 0 ||
                compareAndSetState(available, remaining))
                return remaining;
        }
    }
}
```

“非公平信号量”类

```
static final class NonfairSync extends Sync {
    private static final long serialVersionUID = -2694183684443567898L;

    NonfairSync(int permits) {
        super(permits);
    }

    protected int tryAcquireShared(int acquires) {
        return nonfairTryAcquireShared(acquires);
    }
}
```

1. 信号量构造函数

```
public Semaphore(int permits) {
    sync = new NonfairSync(permits);
}

public Semaphore(int permits, boolean fair) {
    sync = fair ? new FairSync(permits) : new NonfairSync(permits);
}
```

从中，我们可以信号量分为“公平信号量(FairSync)”和“非公平信号量(NonfairSync)”。Semaphore(int permits)函数会默认创建“非公平信号量”。

2. 公平信号量获取和释放

2.1 公平信号量的获取
Semaphore中的公平信号量是FairSync。它的获取API如下：

```
public void acquire() throws InterruptedException {
    sync.acquireSharedInterruptibly(1);
}

public void acquire(int permits) throws InterruptedException {
    if (permits < 0) throw new IllegalArgumentException();
    sync.acquireSharedInterruptibly(permits);
}
```

信号量中的acquire()获取函数，实际上是调用的AQS中的acquireSharedInterruptibly()。

acquireSharedInterruptibly()的源码如下：

```
public final void acquireSharedInterruptibly(int arg)
        throws InterruptedException {
    // 如果线程是中断状态，则抛出异常。
    if (Thread.interrupted())
        throw new InterruptedException();
    // 否则，尝试获取“共享锁”；获取成功则直接返回，获取失败，则通过doAcquireSharedInterruptibly()获取。
    if (tryAcquireShared(arg) < 0)
        doAcquireSharedInterruptibly(arg);
}
```

Semaphore中”公平锁“对应的tryAcquireShared()实现如下：

```
protected int tryAcquireShared(int acquires) {
    for (;;) {
        // 判断“当前线程”是不是CLH队列中的第一个线程线程，
        // 若是的话，则返回-1。
        if (hasQueuedPredecessors())
            return -1;
        // 设置“可以获得的信号量的许可数”
        int available = getState();
        // 设置“获得acquires个信号量许可之后，剩余的信号量许可数”
        int remaining = available - acquires;
        // 如果“剩余的信号量许可数>=0”，则设置“可以获得的信号量许可数”为remaining。
        if (remaining < 0 ||
            compareAndSetState(available, remaining))
            return remaining;
    }
}
```

说明：tryAcquireShared()的作用是尝试获取acquires个信号量许可数。
对于Semaphore而言，state表示的是“当前可获得的信号量许可数”。

下面看看AQS中doAcquireSharedInterruptibly()的实现：

```
private void doAcquireSharedInterruptibly(long arg)
    throws InterruptedException {
    // 创建”当前线程“的Node节点，且Node中记录的锁是”共享锁“类型；并将该节点添加到CLH队列末尾。
    final Node node = addWaiter(Node.SHARED);
    boolean failed = true;
    try {
        for (;;) {
            // 获取上一个节点。
            // 如果上一节点是CLH队列的表头，则”尝试获取共享锁“。
            final Node p = node.predecessor();
            if (p == head) {
                long r = tryAcquireShared(arg);
                if (r >= 0) {
                    setHeadAndPropagate(node, r);
                    p.next = null; // help GC
                    failed = false;
                    return;
                }
            }
            // 当前线程一直等待，直到获取到共享锁。
            // 如果线程在等待过程中被中断过，则再次中断该线程(还原之前的中断状态)。
            if (shouldParkAfterFailedAcquire(p, node) &&
                parkAndCheckInterrupt())
                throw new InterruptedException();
        }
    } finally {
        if (failed)
            cancelAcquire(node);
    }
}
```

说明：doAcquireSharedInterruptibly()会使当前线程一直等待，直到当前线程获取到共享锁(或被中断)才返回。
(01) addWaiter(Node.SHARED)的作用是，创建”当前线程“的Node节点，且Node中记录的锁的类型是”共享锁“(Node.SHARED)；并将该节点添加到CLH队列末尾。关于Node和CLH在”Java多线程系列–“JUC锁”03之 公平锁(一)”已经详细介绍过，这里就不再重复说明了。
(02) node.predecessor()的作用是，获取上一个节点。如果上一节点是CLH队列的表头，则”尝试获取共享锁“。
(03) shouldParkAfterFailedAcquire()的作用和它的名称一样，如果在尝试获取锁失败之后，线程应该等待，则返回true；否则，返回false。
(04) 当shouldParkAfterFailedAcquire()返回ture时，则调用parkAndCheckInterrupt()，当前线程会进入等待状态，直到获取到共享锁才继续运行。
doAcquireSharedInterruptibly()中的shouldParkAfterFailedAcquire(), parkAndCheckInterrupt等函数在”Java多线程系列–“JUC锁”03之 公平锁(一)”中介绍过，这里也就不再详细说明了。

2.2 公平信号量的释放

Semaphore中公平信号量(FairSync)的释放API如下：

```
public void release() {
    sync.releaseShared(1);
}

public void release(int permits) {
    if (permits < 0) throw new IllegalArgumentException();
    sync.releaseShared(permits);
}
```

信号量的releases()释放函数，实际上是调用的AQS中的releaseShared()。

releaseShared()在AQS中实现，源码如下：

```
public final boolean releaseShared(int arg) {
    if (tryReleaseShared(arg)) {
        doReleaseShared();
        return true;
    }
    return false;
}
```

说明：releaseShared()的目的是让当前线程释放它所持有的共享锁。
它首先会通过tryReleaseShared()去尝试释放共享锁。尝试成功，则直接返回；尝试失败，则通过doReleaseShared()去释放共享锁。

Semaphore重写了tryReleaseShared()，它的源码如下：

```
protected final boolean tryReleaseShared(int releases) {
    for (;;) {
        // 获取“可以获得的信号量的许可数”
        int current = getState();
        // 获取“释放releases个信号量许可之后，剩余的信号量许可数”
        int next = current + releases;
        if (next < current) // overflow
            throw new Error("Maximum permit count exceeded");
        // 设置“可以获得的信号量的许可数”为next。
        if (compareAndSetState(current, next))
            return true;
    }
}
```

如果tryReleaseShared()尝试释放共享锁失败，则会调用doReleaseShared()去释放共享锁。doReleaseShared()的源码如下：

```
private void doReleaseShared() {
    for (;;) {
        // 获取CLH队列的头节点
        Node h = head;
        // 如果头节点不为null，并且头节点不等于tail节点。
        if (h != null && h != tail) {
            // 获取头节点对应的线程的状态
            int ws = h.waitStatus;
            // 如果头节点对应的线程是SIGNAL状态，则意味着“头节点的下一个节点所对应的线程”需要被unpark唤醒。
            if (ws == Node.SIGNAL) {
                // 设置“头节点对应的线程状态”为空状态。失败的话，则继续循环。
                if (!compareAndSetWaitStatus(h, Node.SIGNAL, 0))
                    continue;
                // 唤醒“头节点的下一个节点所对应的线程”。
                unparkSuccessor(h);
            }
            // 如果头节点对应的线程是空状态，则设置“文件点对应的线程所拥有的共享锁”为其它线程获取锁的空状态。
            else if (ws == 0 &&
                     !compareAndSetWaitStatus(h, 0, Node.PROPAGATE))
                continue;                // loop on failed CAS
        }
        // 如果头节点发生变化，则继续循环。否则，退出循环。
        if (h == head)                   // loop if head changed
            break;
    }
}
```

说明：doReleaseShared()会释放“共享锁”。它会从前往后的遍历CLH队列，依次“唤醒”然后“执行”队列中每个节点对应的线程；最终的目的是让这些线程释放它们所持有的信号量。

3 非公平信号量获取和释放

Semaphore中的非公平信号量是NonFairSync。在Semaphore中，“非公平信号量许可的释放(release)”与“公平信号量许可的释放(release)”是一样的。
不同的是它们获取“信号量许可”的机制不同，下面是非公平信号量获取信号量许可的代码。

非公平信号量的tryAcquireShared()实现如下：

```
protected int tryAcquireShared(int acquires) {
    return nonfairTryAcquireShared(acquires);
}
```

nonfairTryAcquireShared()的实现如下：

```
final int nonfairTryAcquireShared(int acquires) {
    for (;;) {
        // 设置“可以获得的信号量的许可数”
        int available = getState();
        // 设置“获得acquires个信号量许可之后，剩余的信号量许可数”
        int remaining = available - acquires;
        // 如果“剩余的信号量许可数>=0”，则设置“可以获得的信号量许可数”为remaining。
        if (remaining < 0 ||
            compareAndSetState(available, remaining))
            return remaining;
    }
}
```

说明：非公平信号量的tryAcquireShared()调用AQS中的nonfairTryAcquireShared()。而在nonfairTryAcquireShared()的for循环中，它都会直接判断“当前剩余的信号量许可数”是否足够；足够的话，则直接“设置可以获得的信号量许可数”，进而再获取信号量。
而公平信号量的tryAcquireShared()中，在获取信号量之前会通过if (hasQueuedPredecessors())来判断“当前线程是不是在CLH队列的头部”，是的话，则返回-1。

# Semaphore示例

```
public class SemaphoreTest1 {
    private static final int SEM_MAX = 10;
    public static void main(String[] args) {
        Semaphore sem = new Semaphore(SEM_MAX);
        //创建线程池
        ExecutorService threadPool = Executors.newFixedThreadPool(3);
        //在线程池中执行任务
        threadPool.execute(new MyThread(sem, 5));
        threadPool.execute(new MyThread(sem, 4));
        threadPool.execute(new MyThread(sem, 7));
        //关闭池
        threadPool.shutdown();
    }
}

class MyThread extends Thread {
    private volatile Semaphore sem;    // 信号量
    private int count;        // 申请信号量的大小

    MyThread(Semaphore sem, int count) {
        this.sem = sem;
        this.count = count;
    }

    public void run() {
        try {
            // 从信号量中获取count个许可
            sem.acquire(count);

            Thread.sleep(2000);
            System.out.println(Thread.currentThread().getName() + " acquire count="+count);
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            // 释放给定数目的许可，将其返回到信号量。
            sem.release(count);
            System.out.println(Thread.currentThread().getName() + " release " + count + "");
        }
    }
}
```

(某一次)运行结果：

```
pool-1-thread-1 acquire count=5
pool-1-thread-2 acquire count=4
pool-1-thread-1 release 5
pool-1-thread-2 release 4
pool-1-thread-3 acquire count=7
pool-1-thread-3 release 7
```

结果说明：信号量sem的许可总数是10个；共3个线程，分别需要获取的信号量许可数是5,4,7。前面两个线程获取到信号量的许可后，sem中剩余的可用的许可数是1；因此，最后一个线程必须等前两个线程释放了它们所持有的信号量许可之后，才能获取到7个信号量许可。

[1]:data:image/png;base64,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
