 #  JUC

### 2.1 Java多线程

### 1.1 Java实现多线程的方式

(1)继承Thread类，重写run函数。

(2)实现Runnable接口

(3)实现Callable接口

##### 三种方式的区别

实现Runnable接口可以避免Java单继承特性而带来的局限；增强程序的健壮性，代码能够被多个线程共享，代码与数据是独立的；适合多个相同程序代码的线程区处理同一资源的情况。

继承Thread类和实现Runnable方法启动线程都是使用start方法，然后JVM虚拟机将此线程放到就绪队列中，如果有处理机可用，则执行run方法。

实现Callable接口要实现call方法，并且线程执行完毕后会有返回值。其他的两种都是重写run方法，没有返回值。

#### 1.1.1 Future

Future就是对于具体的Runnable或者Callable任务的执行结果进行取消、查询是否完成、获取结果。必要时可以通过get方法获取执行结果，该方法会阻塞直到任务返回结果。

Future提供了三种功能：

1）判断任务是否完成；

2）能够中断任务；

3）能够获取任务执行结果。

### 1.2 Java多线程交互协作(wait/notify/notifyAll/sleep/yield/join)

wait()方法的作用是将当前运行的线程挂起（即让其进入阻塞状态），直到notify或notifyAll方法来唤醒线程

notify/notifyAll()在同一对象上去调用notify/notifyAll方法，
就可以唤醒对应对象monitor上等待的线程

sleep方法的作用是让当前线程暂停指定的时间

yield方法的作用是暂停当前线程，以便其他线程有机会执行，
不过不能指定暂停的时间，并且也不能保证当前线程马上停止

join方法的作用是父线程等待子线程执行完成后再执行，
换句话说就是将异步执行的线程合并为同步的线程。

### 1.3 Java线程启动(start/run)

start()方法来启动线程，真正实现了多线程运行。这时无需等待run方法体代码执行完毕，可以直接继续执行下面的代码；通过调用Thread类的start()方法来启动一个线程， 这时此线程是处于就绪状态， 并没有运行。 然后通过此Thread类调用方法run()来完成其运行操作的， 这里方法run()称为线程体，它包含了要执行的这个线程的内容， Run方法运行结束， 此线程终止。然后CPU再调度其它线程。

run()方法当作普通方法的方式调用。程序还是要顺序执行，要等待run方法体执行完毕后，才可继续执行下面的代码； 程序中只有主线程——这一个线程， 其程序执行路径还是只有一条， 这样就没有达到写线程的目的。

### 1.4 Java线程终止

1.使用退出标志终止线程

2.使用interrupt()方法中断当前线程

使用interrupt()方法来中断线程有两种情况：

(1)线程处于阻塞状态，如使用了sleep,同步锁的wait,socket中的receiver,
accept等方法时，会使线程处于阻塞状态。当调用线程的interrupt()方法时，
会抛出InterruptException异常。阻塞中的那个方法抛出这个异常，
通过代码捕获该异常，然后break跳出循环状态，从而让我们有机会结束这个线程的执行。
通常很多人认为只要调用interrupt方法线程就会结束，实际上是错的，
一定要先捕获InterruptedException异常之后通过break来跳出循环，
才能正常结束run方法。

(2)线程未处于阻塞状态，使用isInterrupted()判断线程的中断标志来退出循环。
当使用interrupt()方法时，中断标志就会置true，和使用自定义的标志来控制循环
是一样的道理。

为什么要区分进入阻塞状态和和非阻塞状态两种情况了，是因为当阻塞状态时，
如果有interrupt()发生，系统除了会抛出InterruptedException异常外，
还会调用interrupted()函数，调用时能获取到中断状态是true的状态，
调用完之后会复位中断状态为false，所以异常抛出之后通过isInterrupted()
是获取不到中断状态是true的状态，从而不能退出循环，因此在线程未进入阻塞的
代码段时是可以通过isInterrupted()来判断中断是否发生来控制循环，
在进入阻塞状态后要通过捕获异常来退出循环。因此使用interrupt()来退出线程
的最好的方式应该是两种情况都要考虑：

3.使用stop方法强行终止线程

不推荐使用，Thread.stop, Thread.suspend, Thread.resume
Runtime.runFinalizersOnExit

这些终止线程运行的方法已经被废弃

## 2 ThreadLocal

ThreadLocal为每个使用变量的线程提供独立的变量副本，所以每一个线程都可以独立地改变自己的副本，而不会影响其它线程所对应的副本。常用于用户登录控制，如记录session信息。

对于同一个static ThreadLocal，不同线程只能从中get，set，remove自己的变量，而不会影响其他线程的变量。
1、ThreadLocal.get: 获取ThreadLocal中当前线程共享变量的值。
2、ThreadLocal.set: 设置ThreadLocal中当前线程共享变量的值。
3、ThreadLocal.remove: 移除ThreadLocal中当前线程共享变量的值。
4、ThreadLocal.initialValue: ThreadLocal没有被当前线程赋值时或当前线程刚调用remove方法后调用get方法，返回此方法值。
用处：保存线程的独立变量。对一个线程类（继承自Thread)

### 2.1 ThreadLocal原理

ThreadLocalMap是ThreadLocal的内部类，没有实现Map接口，用独立的方式实现了Map的功能，其内部的Entry也独立实现。

![avatar][ThreadLocal类图]

在ThreadLocalMap中，也是用Entry来保存K-V结构数据的。但是Entry中key只能是ThreadLocal对象，这点被Entry的构造方法已经限定死了。

```
static class Entry extends WeakReference<ThreadLocal> {
    /** The value associated with this ThreadLocal. */
    Object value;

    Entry(ThreadLocal k, Object v) {
        super(k);
        value = v;
    }
}
```

Entry继承自WeakReference（弱引用，生命周期只能存活到下次GC前），但只有Key是弱引用类型的，Value并非弱引用。

ThreadLocalMap的成员变量：

```
static class ThreadLocalMap {
    /**
     * The initial capacity -- MUST be a power of two.
     */
    private static final int INITIAL_CAPACITY = 16;

    /**
     * The table, resized as necessary.
     * table.length MUST always be a power of two.
     */
    private Entry[] table;

    /**
     * The number of entries in the table.
     */
    private int size = 0;

    /**
     * The next size value at which to resize.
     */
    private int threshold; // Default to 0
}
```

### 2.2 Hash冲突解决方案

和HashMap的最大的不同在于，ThreadLocalMap结构非常简单，没有next引用，也就是说ThreadLocalMap中解决Hash冲突的方式并非链表的方式，而是采用线性探测的方式，所谓线性探测，就是根据初始key的hashcode值确定元素在table数组中的位置，如果发现这个位置上已经有其他key值的元素被占用，则利用固定的算法寻找一定步长的下个位置，依次判断，直至找到能够存放的位置。

ThreadLocalMap解决Hash冲突的方式就是简单的步长加1或减1，寻找下一个相邻的位置。

显然ThreadLocalMap采用线性探测的方式解决Hash冲突的效率很低，如果有大量不同的ThreadLocal对象放入map中时发送冲突，或者发生二次冲突，则效率很低。

所以这里引出的良好建议是：每个线程只存一个变量，这样的话所有的线程存放到map中的Key都是相同的ThreadLocal，如果一个线程要保存多个变量，就需要创建多个ThreadLocal，多个ThreadLocal放入Map中时会极大的增加Hash冲突的可能。

### 2.3 ThreadLocalMap的内存泄露问题与避免

由于ThreadLocalMap的key是弱引用，而Value是强引用。这就导致了一个问题，ThreadLocal在没有外部对象强引用时，发生GC时弱引用Key会被回收，而Value不会回收，如果创建ThreadLocal的线程一直持续运行，那么这个Entry对象中的value就有可能一直得不到回收，发生内存泄露。

既然Key是弱引用，那么我们要做的事，就是在调用ThreadLocal的get()、set()方法时完成后再调用remove方法，将Entry节点和Map的引用关系移除，这样整个Entry对象在GC Roots分析后就变成不可达了，下次GC的时候就可以被回收。

如果使用ThreadLocal的set方法之后，没有显式的调用remove方法，就有可能发生内存泄露，所以养成良好的编程习惯十分重要，使用完ThreadLocal之后，记得调用remove方法。


## 3 AbstractQueuedSynchronizer(AQS)

同步器一般包含2种方法，一种是acquire，另一种是release。acquire操作阻塞线程，获取锁。release通过某种方式改变让被acquire阻塞的线程继续执行，释放锁。为了实现这2种操作，需要以下3个基本组件的相互协作：

(1)同步状态的原子性管理
(2)线程的阻塞和解除阻塞
(3)队列管理

### 3.1 同步状态的原子性管理

（1）AQS使用一个int State变量来保存同步状态，通过compare-and-swap(CAS)来读取或更新这个状态，并且用了volatile来修饰，保证了在多线程环境下的可见性。

（2）这里的同步状态用int而非long，主要是因为64位long字段的原子性操作在很多平台上是使用内部锁的方式来模拟实1现的，这会使得同步器的会有性能问题。

### 3.2 线程的阻塞和解除阻塞

JDK1.5之前，阻塞线程和解除线程阻塞都是基于Java自身的监控器。在AQS中实现阻塞是用java.util.concurrent包的LockSuport类。方法LockSupport.park阻塞当前线程，直到有个LockSupport.unpark方法被调用。

### 3.3 队列管理

AQS框架关键就在于如何管理被阻塞的线程队列。提供了2个队列，分别是线程安全Sync Queue(CLH Queue)、普通的Condition Queue。

##### Sync Queue

Sync Queue是基于FIFO的队列，用于构建锁或者其他相关同步装置。CLH锁可以更容易地去实现取消（cancellation）和超时功能。

队列中的元素Node是保存线程的引用和线程状态。Node是AQS的一个静态内部类：

Node类的成员变量主要负责保存线程引用、队列的前继和后继节点，以及同步状态：

|成员|描述|
|--|--|
|waitStatus|用来标记Node的状态:CANCELLED:1, 表示当前线程已经被取消SIGNAL:-1,表示当前节点的后继节点等待运行CONDITION:-2, 表示当前节点已被加入Condition QueuePROPAGATE:-3, 共享锁的最终状态是PROPAGATE|
|thread|当前获取lock的线程|
|SHARED|表示节点是共享模式|
|EXCLUSIVE|表示节点是独占模式|
|prev|前继节点|
|next|后继节点|
|nextWaiter|存储Condition Queue中的后继节点|

Node元素是Sync Queue构建的基础。当获取锁的时候，请求形成节点挂载在尾部。而锁资源的释放再获取的过程是从开始向后进行的。

## 4 JUC tools

### 4.1 CountDownLatch

#### 4.1.1 CountDownLatch使用场景

CountDownLatch可以唤醒多个等待的线程。到达自己预期状态的线程会调用CountDownLatch的countDown方法，而等待的线程会调用CountDownLatch的await方法。

![avatar][CountDownLatch使用场景]

```
public static void main(String[] args) throws InterruptedException {
        CountDownLatch countDown = new CountDownLatch(1);
        CountDownLatch await = new CountDownLatch(5);

        // 依次创建并启动处于等待状态的5个MyRunnable线程
        for (int i = 0; i < 5; ++i) {
            new Thread(new MyRunnable(countDown, await)).start();
        }

        System.out.println("用于触发处于等待状态的线程开始工作......");
        countDown.countDown();
        await.await();
        System.out.println("Bingo!");
}

public class MyRunnable implements Runnable {

    private final CountDownLatch countDown;
    private final CountDownLatch await;

    public MyRunnable(CountDownLatch countDown, CountDownLatch await) {
        this.countDown = countDown;
        this.await = await;
    }

    public void run() {
        try {
            countDown.await();//等待主线程执行完毕，获得开始执行信号...
            System.out.println("处于等待的线程开始自己预期工作......");
            await.countDown();//完成预期工作，发出完成信号...
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}

运行结果：
用于触发处于等待状态的线程开始工作......
处于等待的线程开始自己预期工作......
处于等待的线程开始自己预期工作......
处于等待的线程开始自己预期工作......
处于等待的线程开始自己预期工作......
处于等待的线程开始自己预期工作......
Bingo!
```

#### 4.1.2 CountDownLatch原理

CountDownLatch通过AQS里面的共享锁来实现的，在创建CountDownLatch时候，会传递一个参数count，该参数是锁计数器的初始状态，表示该共享锁能够被count个线程同时获取。当某个线程调用CountDownLatch对象的await方法时候，该线程会等待共享锁可获取时，才能获取共享锁继续运行，而共享锁可获取的的条件是state == 0，而锁倒数计数器的初始值为count，每当一个线程调用该CountDownLatch对象的countDown()方法时候，计数器才-1，所以必须有count个线程调用该countDown()方法后，锁计数器才为0，这个时候等待的线程才能继续运行。

### 4.2 CyclicBarrier

#### 3.2.1 CyclicBarrier使用场景

CyclicBarrier允许一组线程互相等待，直到到达某个公共屏障点。与CountDownLatch不同的是该barrier在释放等待线程后可以重用，所以称它为循环（Cyclic）的屏障（Barrier）。

对于失败的同步尝试，CyclicBarrier 使用了一种要么全部要么全不 (all-or-none) 的破坏模式：如果因为中断、失败或者超时等原因，导致线程过早地离开了屏障点，那么在该屏障点等待的其他所有线程也将通过 BrokenBarrierException（如果它们几乎同时被中断，则用 InterruptedException）以反常的方式离开。

```
public static void main(String[] args) {
  int N = 4;
  CyclicBarrier barrier  = new CyclicBarrier(N);
  for(int i=0;i<N;i++)
      new Writer(barrier).start();
}
static class Writer extends Thread {
    private CyclicBarrier cyclicBarrier;
    public Writer(CyclicBarrier cyclicBarrier) {
        this.cyclicBarrier = cyclicBarrier;
    }

    @Override
    public void run() {
        System.out.println("线程"+Thread.currentThread().getName()+"正在写入数据...");
        try {
            Thread.sleep(5000);      //以睡眠来模拟写入数据操作
            System.out.println("线程"+Thread.currentThread().getName()+"写入数据完毕，等待其他线程写入完毕");
            cyclicBarrier.await();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }catch(BrokenBarrierException e){
            e.printStackTrace();
        }
        System.out.println("所有线程写入完毕，继续处理其他任务...");
    }
}
线程Thread-0正在写入数据...
线程Thread-3正在写入数据...
线程Thread-2正在写入数据...
线程Thread-1正在写入数据...
线程Thread-2写入数据完毕，等待其他线程写入完毕
线程Thread-0写入数据完毕，等待其他线程写入完毕
线程Thread-3写入数据完毕，等待其他线程写入完毕
线程Thread-1写入数据完毕，等待其他线程写入完毕
所有线程写入完毕，继续处理其他任务...
所有线程写入完毕，继续处理其他任务...
所有线程写入完毕，继续处理其他任务...
所有线程写入完毕，继续处理其他任务...
```

栅栏：一个同步辅助类，它允许一组线程互相等待，直到到达某个公共屏障点。然后栅栏将打开，所有线程将通过栅栏继续执行。

#### 4.2.2 CyclicBarrier原理

基于ReentrantLock和Condition机制实现。除了getParties()方法，CyclicBarrier的其他方法都需要获取锁。

### 4.3 Semaphone

Semaphore类是一个计数信号量，必须由获取它的线程释放，通常用于限制可以访问某些资源（物理或逻辑的）线程数目。

### 4.3.1 Semaphone使用场景

Semaphore可以用来做流量分流，特别是对公共资源有限的场景，比如数据库连接。
假设有这个的需求，读取几万个文件的数据到数据库中，由于文件读取是IO密集型任务，可以启动几十个线程并发读取，但是数据库连接数只有10个，这时就必须控制最多只有10个线程能够拿到数据库连接进行操作。这个时候，就可以使用Semaphore做流量控制。

```
public class SemaphoreTest {
    private static final int COUNT = 40;
    private static Executor executor = Executors.newFixedThreadPool(COUNT);
    private static Semaphore semaphore = new Semaphore(10);
    public static void main(String[] args) {
        for (int i=0; i< COUNT; i++) {
            executor.execute(new ThreadTest.Task());
        }
    }

    static class Task implements Runnable {
        @Override
        public void run() {
            try {
                //读取文件操作
                semaphore.acquire();
                // 存数据过程
                semaphore.release();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
            }
        }
    }
}
```

### 4.3.2 Semaphone原理

Semaphore实现主要基于AQS，分为非公平策略和公平策略

## 5 JUC locks

### 5.1 ReentrantLock

#### 5.1.1 ReentrantLock使用

ReentrantLock是完全互斥排他的，效率不高。

(1)读锁，此时多个线程可以或得读锁
(2)写锁，此时只有一个线程能获得写锁
(3)读锁和写锁是互斥的

#### 5.1.2 ReentrantLock原理

ReentrantLock是基于AQS实现的

### 5.2 Condition

Condition是在java 1.5中才出现的，它用来替代传统的Object的wait()、notify()实现线程间的协作，相比使用Object的wait()、notify()，使用Condition的await()、signal()这种方式实现线程间协作更加安全和高效。因此通常来说比较推荐使用Condition。

```
class BoundedBuffer {
    final Lock lock = new ReentrantLock();// 锁对象
    final Condition notFull = lock.newCondition(); //写线程条件
    final Condition notEmpty = lock.newCondition();//读线程条件
    final Object[] items = new Object[100];// 初始化一个长度为100的队列
    int putptr/* 写索引 */, takeptr/* 读索引 */, count/* 队列中存在的数据个数 */;

    public void put(Object x) throws InterruptedException {
        lock.lock(); //获取锁
        try {
            while (count == items.length)
                notFull.await();// 当计数器count等于队列的长度时，不能再插入，因此等待。阻塞写线程。
            items[putptr] = x;//赋值
            putptr++;

            if (putptr == items.length)
                putptr = 0;// 若写索引写到队列的最后一个位置了，将putptr置为0。
            count++; // 每放入一个对象就将计数器加1。
            notEmpty.signal(); // 一旦插入就唤醒取数据线程。
        } finally {
            lock.unlock(); // 最后释放锁
        }
    }

    public Object take() throws InterruptedException {
        lock.lock(); // 获取锁
        try {
            while (count == 0)
                notEmpty.await(); // 如果计数器等于0则等待，即阻塞读线程。
            Object x = items[takeptr]; // 取值
            takeptr++;
            if (takeptr == items.length)
                takeptr = 0; //若读锁应读到了队列的最后一个位置了，则读锁应置为0；即当takeptr达到队列长度时，从零开始取
            count++; // 每取一个将计数器减1。
            notFull.signal(); //枚取走一个就唤醒存线程。
            return x;
        } finally {
            lock.unlock();// 释放锁
        }
    }

}
```

### 5.3 LockSupport

LockSupport 和 CAS 是Java并发包中很多并发工具控制机制的基础，它们底层其实都是依赖Unsafe实现。

LockSupport是用来创建锁和其他同步类的基本线程阻塞原语。

LockSupport 提供park()和unpark()方法实现阻塞线程和解除线程阻塞

LockSupport和每个使用它的线程都与一个许可(permit)关联。permit相当于1，0的开关，默认是0，调用一次unpark就加1变成1，调用一次park会消费permit, 也就是将1变成0，同时park立即返回。再次调用park会变成block（因为permit为0了，会阻塞在这里，直到permit变为1）, 这时调用unpark会把permit置为1。每个线程都有一个相关的permit, permit最多只有一个，重复调用unpark也不会积累。

park()和unpark()不会有 “Thread.suspend和Thread.resume所可能引发的死锁” 问题，由于许可的存在，调用 park 的线程和另一个试图将其 unpark 的线程之间的竞争将保持活性。

如果调用线程被中断，则park方法会返回。同时park也拥有可以设置超时时间的版本。

需要特别注意的一点：park 方法还可以在其他任何时间“毫无理由”地返回，因此通常必须在重新检查返回条件的循环里调用此方法。从这个意义上说，park 是“忙碌等待”的一种优化，它不会浪费这么多的时间进行自旋，但是必须将它与 unpark 配对使用才更高效。

三种形式的 park 还各自支持一个 blocker 对象参数。此对象在线程受阻塞时被记录，以允许监视工具和诊断工具确定线程受阻塞的原因。（这样的工具可以使用方法 getBlocker(java.lang.Thread) 访问 blocker。）建议最好使用这些形式，而不是不带此参数的原始形式。在锁实现中提供的作为 blocker 的普通参数是 this。

### 5.4 Unsafe

Unsafe类提供了硬件级别的原子操作，主要提供了以下功能：

1、通过Unsafe类可以分配内存，可以释放内存；

类中提供的3个本地方法allocateMemory、reallocateMemory、freeMemory分别用于分配内存，扩充内存和释放内存

2、可以定位对象某字段的内存位置，也可以修改对象的字段值，即使它是私有的；

JAVA中对象的字段的定位可能通过staticFieldOffset方法实现，该方法返回给定field的内存地址偏移量，这个值对于给定的filed是唯一的且是固定不变的。

getIntVolatile方法获取对象中offset偏移地址对应的整型field的值,支持volatile load语义。

getLong方法获取对象中offset偏移地址对应的long型field的值

3、挂起与恢复

将一个线程进行挂起是通过park方法实现的，调用 park后，线程将一直阻塞直到超时或者中断等条件出现。unpark可以终止一个挂起的线程，使其恢复正常。整个并发框架中对线程的挂起操作被封装在 LockSupport类中，LockSupport类中有各种版本pack方法，但最终都调用了Unsafe.park()方法。

4、CAS操作

是通过compareAndSwapXXX方法实现的

CAS操作有3个操作数，内存值M，预期值E，新值U，如果M==E，则将内存值修改为B，否则啥都不做。

ABA问题：

比如说一个线程one从内存位置V中取出A，这时候另一个线程two也从内存中取出A，并且two进行了一些操作变成了B，然后two又将V位置的数据变成A，这时候线程one进行CAS操作发现内存中仍然是A，然后one操作成功。
尽管线程one的CAS操作成功，但是不代表这个过程就是没有问题的。

ABA问题解决：

引用和版本号，通过版本号来区别节点的循环使用

## 5 JUC atomic

atomic方便在多线程环境下无锁的进行原子操作。原子变量的底层使用了处理器提供的原子指令，但是不同的CPU架构可能提供的原子指令不一样，也有可能需要某种形式的内部锁,所以该方法不能绝对保证线程不被阻塞。

在Atomic包里一共有12个类，四种原子更新方式，分别是原子更新基本类型，原子更新数组，原子更新引用和原子更新字段。Atomic包里的类基本都是使用Unsafe实现的包装类。

### 5.1 原子更新基本类型类

AtomicBoolean：原子更新布尔类型。
AtomicInteger：原子更新整型。
AtomicLong：原子更新长整型。

### 5.2 原子更新数组类

AtomicIntegerArray：原子更新整型数组里的元素。
AtomicLongArray：原子更新长整型数组里的元素。
AtomicReferenceArray：原子更新引用类型数组里的元素。

### 5.3 原子更新引用类型

AtomicReference：原子更新引用类型。
AtomicReferenceFieldUpdater：原子更新引用类型里的字段。
AtomicMarkableReference：原子更新带有标记位的引用类型。可以原子的更新一个布尔类型的标记位和引用类型。构造方法是AtomicMarkableReference(V initialRef, boolean initialMark)

## 6 JUC Colletions

### 6.1 CopyOnWriteArrayList & CopyOnWriteArraySet

#### 6.1.1 CopyOnWriteArrayList & CopyOnWriteArraySet 使用场景

java中，List在遍历的时候，如果被修改了会抛出java.util.ConcurrentModificationException错误。

那么有没有办法在遍历一个list的时候，还向list中添加元素呢？办法是有的。就是java concurrent包中的CopyOnWriteArrayList。

CopyOnWriteArrayList类最大的特点就是，在对其实例进行修改操作（add/remove等）会新建一个数据并修改，修改完毕之后，再将原来的引用指向新的数组。这样，修改过程没有修改原来的数组。也就没有了ConcurrentModificationException错误。

#### 6.1.1 CopyOnWriteArrayList & CopyOnWriteArraySet 原理

CopyOnWriteArrayList实现：
（1）ReentrantLock加锁，保证线程安全
（2）拷贝原容器，长度为原容器长度加一
（3）在新副本上执行添加操作
（4）将原容器引用指向新副本

CopyOnWriteArraySet实现：
基于CopyOnWriteArrayList实现，其唯一的不同是在add时调用的是CopyOnWriteArrayList的addIfAbsent方法，其遍历当前Object数组，如Object数组中已有了当前元素，则直接返回，如果没有则放入Object数组的尾部，并返回。

### 6.2 ConcurrentHashMap

#### 6.2.1 ConcurrentHashMap应用场景

多线程对HashMap数据添加删除操作时，可以采用ConcurrentHashMap

#### 6.2.2 ConcurrentHashMap 1.7

每一个segment都是一个HashEntry<K,V>[] table， table中的每一个元素本质上都是一个HashEntry的单向队列。

![avatar][ConcurrentHashMap 1.7]

#### 6.2.3 ConcurrentHashMap 1.8

![avatar][ConcurrentHashMap 1.8]

每一个segment都是一个HashEntry<K,V>[] table， table中的每一个元素本质上都是一个HashEntry的单向队列。当队列元素过多时变成红黑树

### 6.3 ConcurrentSkipListMap

#### 6.3.1 ConcurrentSkipListMap应用场景

ConcurrentSkipListMap是“线程安全”的哈希表，而TreeMap是非线程安全的。

#### 6.3.2 ConcurrentSkipListMap原理

ConcurrentSkipListMap存储结构跳跃表（SkipList）：
1、最底层的数据节点按照关键字升序排列。
2、包含多级索引，每个级别的索引节点按照其关联数据节点的关键字升序排列。
3、高级别索引是其低级别索引的子集。
4、如果关键字key在级别level=i的索引中出现，则级别level<=i的所有索引中都包含key。
注：类比一下数据库的索引、B+树。

![avatar][ConcurrentSkipListMap原理]

### 6.4 ArrayBlockingQueue

#### 6.4.1 ArrayBlockingQueue应用场景

充当生产者&消费者的中间数据结构

#### 6.4.2 ArrayBlockingQueue原理

ArrayBlockingQueue底层实现是通过数组来实现的，按照先进先出的原则，通过ReentrantLock lock 重入锁来锁住当前竞争资源，使用Condition notEmpty，Condition notFull来实现生产者-消费者模式(通知模式)。

### 6.5 LinkedBlockingQueue & LinkedBlockingDeque

### 6.5.1 LinkedBlockingQueue & LinkedBlockingDeque应用场景

充当生产者&消费者的中间数据结构

### 6.5.1 LinkedBlockingQueue & LinkedBlockingDeque原理

LinkedBlockingQueue底层实现是通过链表来实现的，按照先进先出的原则，通过ReentrantLock lock 重入锁来锁住当前竞争资源，使用Condition notEmpty，Condition notEmpty来实现生产者-消费者模式(通知模式)。

### 6.5.2 LinkedBlockingQueue & LinkedBlockingDeque区别

两个都是队列，只不过前者只能一端出一端入，后者则可以两端同时出入，并且都是结构改变线程安全的队列。

### 6.6 DelayQueue

DelayQueue是一个无界阻塞队列，用于放置实现了Delayed接口的对象，只有在延迟期满时才能从中提取元素。该队列时有序的，即队列的头部是延迟期满后保存时间最长的Delayed 元素。注意：不能将null元素放置到这种队列中。

### 6.6.1 DelayQueue原理

DelayQueue 将实现了 Delayed 接口的对象添加到PriorityQueue中，通过在依赖内聚ReentrantLock重入锁的 Condition 上调用 await(delayTime) 方法，实现了延迟获取阻塞队列中元素的功能。

### 6.7 PriortityBlockingQueue

#### 6.7.1 PriortityBlockingQueue应用场景

#### 6.7.2 PriortityBlockingQueue原理

PriortityBlockingQueue底层实现是通过PriorityQueue来实现的，通过ReentrantLock lock 重入锁来锁住当前竞争资源，使用Condition notEmpty来实现生产者-消费者模式(通知模式)。

### 6.8 SynchronousQueue

SynchronousQueue是一种阻塞队列，其中每个插入操作必须等待另一个线程的对应移除操作 ，反之亦然。同步队列没有任何内部容量，甚至连一个队列的容量都没有。

### 6.8.1 SynchronousQueue实现原理

SynchronousQueue直接使用CAS实现线程的安全访问。

公平模式下，底层实现使用的是TransferQueue这个内部队列，它有一个head和tail指针，用于指向当前正在等待匹配的线程节点。
非公平模式底层的实现使用的是TransferStack， 一个栈，实现中用head指针指向栈顶.

### 6.9 LinkedTransferQueue

LinkedTransferQueue和SynchronousQueue类似，可以把它看成是SynchronousQueue和LinkedBlockingQueue的超集。

#### 6.9.1 LinkedTransferQueue原理

LinkedTransferQueue通过CAS，如果队列中有人发现有人在等，则直接给那个人（有一个参数waiter指定了在等的线程）如果没人在等，就放进队列

### 6.10 ConcurrentLinkedQueue & ConcurrentLinkedDeque

一个基于链接节点的无界线程安全队列。此队列按照 FIFO（先进先出）原则对元素进行排序。

#### 6.10.1 ConcurrentLinkedQueue & ConcurrentLinkedDeque原理

使用CAS算法进行出队和入队

## 7 线程池ThreadPool

### 7.1 线程池ThreadPool种类

public static ExecutorService newFixedThreadPool(int nThreads)

创建固定数目线程的线程池，每当提交一个任务时就创建一个线程，直到达到线程池的最大数量，这时线程池的规模将不会再变化(如果某个线程由于发生的Exception而结束，那么线程池会补充一个新的线程)

public static ExecutorService newCachedThreadPool()

创建一个可缓存的线程池，调用execute 将重用以前构造的线程（如果线程可用）。如果现有线程没有可用的，则创建一个新线程并添加到池中。终止并从缓存中移除那些已有 60 秒钟未被使用的线程。

public static ExecutorService newSingleThreadExecutor()

创建一个单线程化的Executor。

public static ScheduledExecutorService newScheduledThreadPool(int corePoolSize)

创建一个固定长度的线程池，而且以延迟或定时的方式来执行任务，类似于Timer。

### 7.2 线程池ThreadPool实现

4种线程池的实现如下：

```
public static ExecutorService newCachedThreadPool() {
		return new ThreadPoolExecutor(0, Integer.MAX_VALUE,
																	60L, TimeUnit.SECONDS,
																	new SynchronousQueue<Runnable>());
}
public static ExecutorService newFixedThreadPool(int nThreads) {
		return new ThreadPoolExecutor(nThreads, nThreads,
																	0L, TimeUnit.MILLISECONDS,
																	new LinkedBlockingQueue<Runnable>());
}
public static ScheduledExecutorService newScheduledThreadPool(int corePoolSize) {
		return new ScheduledThreadPoolExecutor(corePoolSize);//这边居然不是直接调ThreadPoolExecutor构造函数，但是我们追一下代码看看下面这个函数你就会明白

}
public class ScheduledThreadPoolExecutor
        extends ThreadPoolExecutor
        implements ScheduledExecutorService 
```

ThreadPoolExecutor的实现：

```
public ThreadPoolExecutor(int corePoolSize,
													int maximumPoolSize,
													long keepAliveTime,
													TimeUnit unit,
													BlockingQueue<Runnable> workQueue) {
		this(corePoolSize, maximumPoolSize, keepAliveTime, unit, workQueue,
				 Executors.defaultThreadFactory(), defaultHandler);
}
```

corePoolSize :线程池的核心池大小，在创建线程池之后，线程池默认没有任何线程。

当有任务过来的时候才会去创建创建线程执行任务。换个说法，线程池创建之后，线程池中的线程数为0，当任务过来就会创建一个线程去执行，直到线程数达到corePoolSize 之后，就会被到达的任务放在队列中。（注意是到达的任务）。换句更精炼的话：corePoolSize 表示允许线程池中允许同时运行的最大线程数。

如果执行了线程池的prestartAllCoreThreads()方法，线程池会提前创建并启动所有核心线程。

maximumPoolSize :线程池允许的最大线程数，他表示最大能创建多少个线程。maximumPoolSize肯定是大于等于corePoolSize。

keepAliveTime :表示线程没有任务时最多保持多久然后停止。默认情况下，只有线程池中线程数大于corePoolSize 时，keepAliveTime 才会起作用。换句话说，当线程池中的线程数大于corePoolSize，并且一个线程空闲时间达到了keepAliveTime，那么就是shutdown。

Unit:keepAliveTime 的单位。

workQueue ：一个阻塞队列，用来存储等待执行的任务，当线程池中的线程数超过它的corePoolSize的时候，线程会进入阻塞队列进行阻塞等待。通过workQueue，线程池实现了阻塞功能

threadFactory ：线程工厂，用来创建线程。

handler :表示当拒绝处理任务时的策略。

### 7.3 任务缓存队列

在前面我们多次提到了任务缓存队列，即workQueue，它用来存放等待执行的任务。

workQueue的类型为BlockingQueue<Runnable>，通常可以取下面三种类型：

1）有界任务队列ArrayBlockingQueue：基于数组的先进先出队列，此队列创建时必须指定大小；

2）无界任务队列LinkedBlockingQueue：基于链表的先进先出队列，如果创建时没有指定此队列大小，则默认为Integer.MAX_VALUE；

3）直接提交队列synchronousQueue：这个队列比较特殊，它不会保存提交的任务，而是将直接新建一个线程来执行新来的任务。

### 7.4 拒绝策略

AbortPolicy:丢弃任务并抛出RejectedExecutionException

CallerRunsPolicy：只要线程池未关闭，该策略直接在调用者线程中，运行当前被丢弃的任务。显然这样做不会真的丢弃任务，但是，任务提交线程的性能极有可能会急剧下降。

DiscardOldestPolicy：丢弃队列中最老的一个请求，也就是即将被执行的一个任务，并尝试再次提交当前任务。

DiscardPolicy：丢弃任务，不做任何处理。

### 7.5 线程池的任务处理策略：

如果当前线程池中的线程数目小于corePoolSize，则每来一个任务，就会创建一个线程去执行这个任务；

如果当前线程池中的线程数目>=corePoolSize，则每来一个任务，会尝试将其添加到任务缓存队列当中，若添加成功，则该任务会等待空闲线程将其取出去执行；若添加失败（一般来说是任务缓存队列已满），则会尝试创建新的线程去执行这个任务；如果当前线程池中的线程数目达到maximumPoolSize，则会采取任务拒绝策略进行处理；

如果线程池中的线程数量大于 corePoolSize时，如果某线程空闲时间超过keepAliveTime，线程将被终止，直至线程池中的线程数目不大于corePoolSize；如果允许为核心池中的线程设置存活时间，那么核心池中的线程空闲时间超过keepAliveTime，线程也会被终止。

## 8 volatile&&JMM

### 8.1 JMM

Java内存模型的主要目标:定义程序中各个变量的访问规则，在JVM中将变量存储到内存和从内存中取出变量。

此处的变量与Java编程里面的变量有所不同，它包含了实例字段、静态字段和构成数组对象的元素，但不包含局部变量和方法参数，因为后者是线程私有的，不会共享，当然不存在数据竞争问题（如果局部变量是一个reference引用类型，它引用的对象在Java堆中可被各个线程共享，但是reference引用本身在Java栈的局部变量表中，是线程私有的）。

为了获得较高的执行效能，Java内存模型并没有限制执行引起使用处理器的特定寄存器或者缓存来和主内存进行交互，也没有限制即时编译器进行调整代码执行顺序这类优化措施。

JMM规定了所有的变量都存储在主内存（Main Memory）中。每个线程还有自己的工作内存（Working Memory）,线程的工作内存中保存了该线程使用到的变量的主内存的副本拷贝，线程对变量的所有操作（读取、赋值等）都必须在工作内存中进行，而不能直接读写主内存中的变量（volatile变量仍然有工作内存的拷贝，但是由于它特殊的操作顺序性规定，所以看起来如同直接在主内存中读写访问一般）。不同的线程之间也无法直接访问对方工作内存中的变量，线程之间值的传递都需要通过主内存来完成。

![avatar][JMM]

线程1和线程2要想进行数据的交换一般要经历下面的步骤：

1.线程1把工作内存1中的更新过的共享变量刷新到主内存中去。

2.线程2到主内存中去读取线程1刷新过的共享变量，然后copy一份到工作内存2中去。

Java内存模型是围绕着并发编程中原子性、可见性、有序性这三个特征来建立的

#### 8.1.1 原子性

原子性:一个操作不能被打断，要么全部执行完毕，要么不执行。

在java中提供了两个高级的字节码指令monitorenter和monitorexit，使用对应的关键字Synchronized来保证代码块内的操作是原子的

#### 8.1.2 可见性

可见性：一个线程对共享变量做了修改之后，其他的线程立即能够看到（感知到）该变量这种修改（变化）。

在Java中可以使用volatile关键字来保证多线程操作时变量的可见性。volatile的功能是被其修饰的变量在被修改后可以立即同步到主内存，而被其修饰的变量在每次使用之前都会从主内存刷新。除此之外，synchronized和ﬁnal两个关键字也可以实现可见性 。volatile也可以看作是轻量级的锁，在其内部使用了Lock指令来解决可见性问题。
volatile关键字修饰的共享变量，在进行写操作的时候会多出一个lock前缀的汇编指令，这个指令会触发总线锁或者缓存锁，通过缓存一致性协议来解决可见性问题。对于声明了volatile的变量进行写操作时，JVM就会向处理器发送一条Lock前缀的指令，把这个变量所在的缓存行的数据写回到系统内存，再根据MESI的缓存一致性协议，来保证多核CPU下的各个高速缓存中的数据的一致性。

#### 8.1.3 有序性

有序性：如果在本线程内观察，所有操作都是有序的；如果一个线程中观察另一个线程，所有的操作都是无序的。前一句是指线程内表现为串行的语义，后一句是指指令重排序现象和工作内存与主内存同步延迟的现象。

在Java中，可以使用synchronized和volatile来保证多线程之间操作的有序性。只是实现方式有所区别： volatile关键字会禁止指令重排，synchronized关键字保证同一时刻只允许一个线程的操作。

#### 8.1.4 happens-before原则

Java内存模型中定义的两项操作之间的次序关系，如果说操作A先行发生于操作B，操作A产生的影响能被操作B观察到，“影响”包含了修改了内存中共享变量的值、发送了消息、调用了方法等。

下面是Java内存模型下一些”天然的“happens-before关系，这些happens-before关系无须任何同步器协助就已经存在，可以在编码中直接使用。如果两个操作之间的关系不在此列，并且无法从下列规则推导出来的话，它们就没有顺序性保障，虚拟机可以对它们进行随意地重排序。

a.程序次序规则(Pragram Order Rule)：在一个线程内，按照程序代码顺序，书写在前面的操作先行发生于书写在后面的操作。准确地说应该是控制流顺序而不是程序代码顺序，因为要考虑分支、循环结构。

b.管程锁定规则(Monitor Lock Rule)：一个unlock操作先行发生于后面对同一个锁的lock操作。这里必须强调的是同一个锁，而”后面“是指时间上的先后顺序。

c.volatile变量规则(Volatile Variable Rule)：对一个volatile变量的写操作先行发生于后面对这个变量的读取操作，这里的”后面“同样指时间上的先后顺序。

d.线程启动规则(Thread Start Rule)：Thread对象的start()方法先行发生于此线程的每一个动作。

e.线程终于规则(Thread Termination Rule)：线程中的所有操作都先行发生于对此线程的终止检测，我们可以通过Thread.join()方法结束，Thread.isAlive()的返回值等作段检测到线程已经终止执行。

f.线程中断规则(Thread Interruption Rule)：对线程interrupt()方法的调用先行发生于被中断线程的代码检测到中断事件的发生，可以通过Thread.interrupted()方法检测是否有中断发生。

g.对象终结规则(Finalizer Rule)：一个对象初始化完成(构造方法执行完成)先行发生于它的finalize()方法的开始。

g.传递性(Transitivity)：如果操作A先行发生于操作B，操作B先行发生于操作C，那就可以得出操作A先行发生于操作C的结论。

### 8.2 volatile

(1)保证被volatile修饰的共享变量对所有线程总数可见的，也就是当一个线程修改了一个被volatile修饰共享变量的值，新值总数可以被其他线程立即得知。关于volatile的可见性作用，我们必须意识到被volatile修饰的变量对所有线程总数立即可见的，对volatile变量的所有写操作总是能立刻反应到其他线程中，但是对于volatile变量运算操作在多线程环境并不保证安全性

(2)禁止指令重排序优化。volatile是通过内存屏障(Memory Barrier）禁止指令重排优化的，内存屏障，又称内存栅栏，是一个CPU指令，它的作用有两个，一是保证特定操作的执行顺序，二是保证某些变量的内存可见性（利用该特性实现volatile的内存可见性）。由于编译器和处理器都能执行指令重排优化。如果在指令间插入一条Memory Barrier则会告诉编译器和CPU，不管什么指令都不能和这条Memory Barrier指令重排序，也就是说通过插入内存屏障禁止在内存屏障前后的指令执行重排序优化。Memory Barrier的另外一个作用是强制刷出各种CPU的缓存数据，因此任何CPU上的线程都能读取到这些数据的最新版本。

## 9 synchronized

### 9.1 synchronized的三种应用方式

修饰实例方法，作用于当前实例加锁，进入同步代码前要获得当前实例的锁

修饰静态方法，作用于当前类对象加锁，进入同步代码前要获得当前类对象的锁

修饰代码块，指定加锁对象，对给定对象加锁，进入同步代码库前要获得给定对象的锁。

### 9.2 synchronized原理

Java对象头与Monitor

在JVM中，对象在内存中的布局分为三块区域：对象头、实例数据和对齐填充。如下：

![avatar][Java对象头与Monitor]

实例变量：存放类的属性数据信息，包括父类的属性信息，如果是数组的实例部分还包括数组的长度，这部分内存按4字节对齐。

填充数据：由于虚拟机要求对象起始地址必须是8字节的整数倍。填充数据不是必须存在的，仅仅是为了字节对齐，这点了解即可。

Java对象头：它实现synchronized的锁对象的基础，一般而言，synchronized使用的锁对象是存储在Java对象头里的，jvm中采用2个字来存储对象头(如果对象是数组则会分配3个字，多出来的1个字记录的是数组长度)，其主要结构是由Mark Word 和 Class Metadata Address 组成，其结构说明如下表：

|虚拟机位数|头对象结构|说明|
|--|--|--|
|32/64bit|Mark Word|存储对象的hashCode、锁信息或分代年龄或GC标志等信息|
|32/64bit|Class Metadata Address|类型指针指向对象的类元数据，JVM通过这个指针确定该对象是哪个类的实例|

其中Mark Word在默认情况下存储着对象的HashCode、分代年龄、锁标记位等以下是32位JVM的Mark Word默认存储结构

|锁状态|锁标志位|25bit|4bit|1bit是否是偏向锁|
|--|--|--|--|--|
|无锁状态|对象HashCode|对象分代年龄|0|01|

由于对象头的信息是与对象自身定义的数据没有关系的额外存储成本，因此考虑到JVM的空间效率，Mark Word 被设计成为一个非固定的数据结构，以便存储更多有效的数据，它会根据对象本身的状态复用自己的存储空间，如32位JVM下，除了上述列出的Mark Word默认存储结构外，还有如下可能变化的结构：

![avatar][Mark Word存储结构]

其中轻量级锁和偏向锁是Java 6 对 synchronized 锁进行优化后新增加的，稍后我们会简要分析。这里我们主要分析一下重量级锁也就是通常说synchronized的对象锁，锁标识位为10，其中指针指向的是monitor对象（也称为管程或监视器锁）的起始地址。每个对象都存在着一个 monitor 与之关联，对象与其 monitor 之间的关系有存在多种实现方式，如monitor可以与对象一起创建销毁或当线程试图获取对象锁时自动生成，但当一个 monitor 被某个线程持有后，它便处于锁定状态。在Java虚拟机(HotSpot)中，monitor是由ObjectMonitor实现的，其主要数据结构如下（位于HotSpot虚拟机源码ObjectMonitor.hpp文件，C++实现的）

```
ObjectMonitor() {
    _header       = NULL;
    _count        = 0; //记录个数
    _waiters      = 0,
    _recursions   = 0;
    _object       = NULL;
    _owner        = NULL;
    _WaitSet      = NULL; //处于wait状态的线程，会被加入到_WaitSet
    _WaitSetLock  = 0 ;
    _Responsible  = NULL ;
    _succ         = NULL ;
    _cxq          = NULL ;
    FreeNext      = NULL ;
    _EntryList    = NULL ; //处于等待锁block状态的线程，会被加入到该列表
    _SpinFreq     = 0 ;
    _SpinClock    = 0 ;
    OwnerIsThread = 0 ;
  }
```

ObjectMonitor中有两个队列，\_WaitSet 和 \_EntryList，用来保存ObjectWaiter对象列表( 每个等待锁的线程都会被封装成ObjectWaiter对象)，\_owner指向持有ObjectMonitor对象的线程，当多个线程同时访问一段同步代码时，首先会进入 \_EntryList 集合，当线程获取到对象的monitor 后进入 \_Owner 区域并把monitor中的owner变量设置为当前线程同时monitor中的计数器count加1，若线程调用 wait() 方法，将释放当前持有的monitor，owner变量恢复为null，count自减1，同时该线程进入 WaitSet集合中等待被唤醒。若当前线程执行完毕也将释放monitor(锁)并复位变量的值，以便其他线程进入获取monitor(锁)。如下图所示

![avatar][ObjectMonitor数据结构]

synchronized锁便是通过这种方式获取锁的，也是为什么Java中任意对象可以作为锁的原因，同时也是notify/notifyAll/wait等方法存在于顶级对象Object中的原因

### 9.3 Java虚拟机对synchronized的优化

偏向锁

在大多数情况下，锁不仅不存在多线程竞争，而且总是由同一线程多次获得，因此为了减少同一线程获取锁(会涉及到一些CAS操作,耗时)的代价而引入偏向锁。偏向锁的核心思想是，如果一个线程获得了锁，那么锁就进入偏向模式，此时Mark Word 的结构也变为偏向锁结构，当这个线程再次请求锁时，无需再做任何同步操作，即获取锁的过程，这样就省去了大量有关锁申请的操作，从而也就提供程序的性能。所以，对于没有锁竞争的场合，偏向锁有很好的优化效果，毕竟极有可能连续多次是同一个线程申请相同的锁。但是对于锁竞争比较激烈的场合，偏向锁就失效了，会先升级为轻量级锁。

轻量级锁

倘若偏向锁失败，此时Mark Word 的结构也变为轻量级锁的结构。轻量级锁能够提升程序性能的依据是“对绝大部分的锁，在整个同步周期内都不存在竞争”，注意这是经验数据。需要了解的是，轻量级锁所适应的场景是线程交替执行同步块的场合，如果存在同一时间访问同一锁的场合，就会导致轻量级锁膨胀为重量级锁。

自旋锁

轻量级锁失败后，虚拟机为了避免线程真实地在操作系统层面挂起，还会进行一项称为自旋锁的优化手段。这是基于在大多数情况下，线程持有锁的时间都不会太长，如果直接挂起操作系统层面的线程可能会得不偿失，毕竟操作系统实现线程之间的切换时需要从用户态转换到核心态，这个状态之间的转换需要相对比较长的时间，时间成本相对较高，因此自旋锁会假设在不久将来，当前的线程可以获得锁，因此虚拟机会让当前想要获取锁的线程做几个空循环(这也是称为自旋的原因)，一般不会太久，可能是50个循环或100循环，在经过若干次循环后，如果得到锁，就顺利进入临界区。如果还不能获得锁，那就会将线程在操作系统层面挂起，这就是自旋锁的优化方式，这种方式确实也是可以提升效率的。最后没办法也就只能升级为重量级锁了。

锁消除

Java虚拟机在JIT编译时(可以简单理解为当某段代码即将第一次被执行时进行编译，又称即时编译)，通过对运行上下文的扫描，去除不可能存在共享资源竞争的锁，通过这种方式消除没有必要的锁，可以节省毫无意义的请求锁时间，如下StringBuffer的append是一个同步方法，但是在add方法中的StringBuffer属于一个局部变量，并且不会被其他线程所使用，因此StringBuffer不可能存在共享资源竞争的情景，JVM会自动将其锁消除。
