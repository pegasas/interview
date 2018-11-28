# CyclicBarrier

概述
　　CyclicBarrier是一个同步工具类，它允许一组线程互相等待，直到到达某个公共屏障点。与CountDownLatch不同的是该barrier在释放等待线程后可以重用，所以称它为循环（Cyclic）的屏障（Barrier）。

　　CyclicBarrier支持一个可选的Runnable命令，在一组线程中的最后一个线程到达之后（但在释放所有线程之前），该命令只在每个屏障点运行一次。若在继续所有参与线程之前更新共享状态，此屏障操作很有用。



```
//parties表示屏障拦截的线程数量，当屏障撤销时，先执行barrierAction，然后在释放所有线程
public CyclicBarrier(int parties, Runnable barrierAction)
//barrierAction默认为null
public CyclicBarrier(int parties)

/*
 *当前线程等待直到所有线程都调用了该屏障的await()方法
 *如果当前线程不是将到达的最后一个线程，将会被阻塞。解除阻塞的情况有以下几种
 *    1）最后一个线程调用await()
 *    2）当前线程被中断
    3）其他正在该CyclicBarrier上等待的线程被中断
    4）其他正在该CyclicBarrier上等待的线程超时
    5）其他某个线程调用该CyclicBarrier的reset()方法
 *如果当前线程在进入此方法时已经设置了该线程的中断状态或者在等待时被中断，将抛出InterruptedException，并且清除当前线程的已中断状态。
 *如果在线程处于等待状态时barrier被reset()或者在调用await()时 barrier 被损坏，将抛出 BrokenBarrierException 异常。
 *如果任何线程在等待时被中断，则其他所有等待线程都将抛出 BrokenBarrierException 异常，并将 barrier 置于损坏状态。 *如果当前线程是最后一个将要到达的线程，并且构造方法中提供了一个非空的屏障操作（barrierAction），那么在允许其他线程继续运行之前，当前线程将运行该操作。如果在执行屏障操作过程中发生异常，则该异常将传播到当前线程中，并将 barrier 置于损坏状态。
 *
 *返回值为当前线程的索引，0表示当前线程是最后一个到达的线程
 */
public int await() throws InterruptedException, BrokenBarrierException
//在await()的基础上增加超时机制，如果超出指定的等待时间，则抛出 TimeoutException 异常。如果该时间小于等于零，则此方法根本不会等待。
public int await(long timeout, TimeUnit unit) throws InterruptedException, BrokenBarrierException, TimeoutException

//将屏障重置为其初始状态。如果所有参与者目前都在屏障处等待，则它们将返回，同时抛出一个BrokenBarrierException。
public void reset()
```

对于失败的同步尝试，CyclicBarrier 使用了一种要么全部要么全不 (all-or-none) 的破坏模式：如果因为中断、失败或者超时等原因，导致线程过早地离开了屏障点，那么在该屏障点等待的其他所有线程也将通过 BrokenBarrierException（如果它们几乎同时被中断，则用 InterruptedException）以反常的方式离开。

使用示例：

每个Worker处理矩阵中的一行，在处理完所有的行之前，该线程将一直在屏障处等待。在各个WOrker处理完所有行后，将执行提供的Runnable屏障操作。

```
class Solver {
    final int N;    //矩阵的行数
    final float[][] data;    //要处理的矩阵
    final CyclicBarrier barrier;    //循环屏障

    class Worker implements Runnable {
        int myRow;
        Worker(int row) { myRow = row; }
        public void run() {
            while (!done()) {
                processRow(myRow);    //处理指定一行数据

                try {
                    barrier.await();     //在屏障处等待直到
                } catch (InterruptedException ex) {
                    return;
                } catch (BrokenBarrierException ex) {
                    return;
                }
            }
        }
    }

    public Solver(float[][] matrix) {
        data = matrix;
        N = matrix.length;
        //初始化CyclicBarrier
        barrier = new CyclicBarrier(N, new Runnable() {
                                           public void run() {
                                             mergeRows(...); //合并行
                                           }
                                       });
        for (int i = 0; i < N; ++i)
            new Thread(new Worker(i)).start();

        waitUntilDone();
    }
}
```

实现原理

基于ReentrantLock和Condition机制实现。除了getParties()方法，CyclicBarrier的其他方法都需要获取锁。

域

```
/** The lock for guarding barrier entry */
private final ReentrantLock lock = new ReentrantLock();    //可重入锁
/** Condition to wait on until tripped */
private final Condition trip = lock.newCondition();
/** The number of parties */
private final int parties;    //拦截的线程数量
/* The command to run when tripped */
private final Runnable barrierCommand;    //当屏障撤销时，需要执行的屏障操作
//当前的Generation。每当屏障失效或者开闸之后都会自动替换掉。从而实现重置的功能。
private Generation generation = new Generation();

/**
 * Number of parties still waiting. Counts down from parties to 0
 * on each generation.  It is reset to parties on each new
 * generation or when broken.
 */
private int count;    //还能阻塞的线程数（即parties-当前阻塞的线程数），当新建generation或generation被破坏时，count会被重置。因为对Count的操作都是在获取锁之后，所以不需要其他同步措施。

//静态内联类
private static class Generation {
    boolean broken = false;    //当前的屏障是否破坏
}
```

await()

```
public int await() throws InterruptedException, BrokenBarrierException {
    try {
        return dowait(false, 0L);
    } catch (TimeoutException toe) {
        throw new Error(toe); // cannot happen;
    }
}

private int dowait(boolean timed, long nanos)
    throws InterruptedException, BrokenBarrierException,
           TimeoutException {
    final ReentrantLock lock = this.lock;
    lock.lock();    //获取锁
    try {
        //保存此时的generation
        final Generation g = generation;
        //判断屏障是否被破坏
        if (g.broken)
            throw new BrokenBarrierException();
        //判断线程是否被中断，如果被中断，调用breakBarrier()进行屏障破坏处理，并抛出InterruptedException
        if (Thread.interrupted()) {
            breakBarrier();
            throw new InterruptedException();
        }

        int index = --count;    //剩余count递减，并赋值给线程索引，作为方法的返回值
        //如果线程索引将为0，说明当前线程是最后一个到达的线程。执行可能存在的屏障操作 barrierCommand，设置下一个Generation。相当于每次开闸之后都进行了一次reset。
        if (index == 0) {  // tripped
            boolean ranAction = false;
            try {
                final Runnable command = barrierCommand;
                if (command != null)
                    command.run();    //同步执行barrierCommand
                ranAction = true;
                nextGeneration();    //执行成功设置下一个nextGeneration
                return 0;
            } finally {
                if (!ranAction)    //如果barrierCommand执行失败，进行屏障破坏处理
                    breakBarrier();
            }
        }

        //如果当前线程不是最后一个到达的线程
        // loop until tripped, broken, interrupted, or timed out
        for (;;) {
            try {
                if (!timed)
                    trip.await();    //调用Condition的await()方法阻塞
                else if (nanos > 0L)
                    nanos = trip.awaitNanos(nanos);    //调用Condition的awaitNanos()方法阻塞
            } catch (InterruptedException ie) {
                //如果当前线程被中断，则判断是否有其他线程已经使屏障破坏。若没有则进行屏障破坏处理，并抛出异常；否则再次中断当前线程
                if (g == generation && ! g.broken) {
                    breakBarrier();
                    throw ie;
                } else {
                    // We're about to finish waiting even if we had not
                    // been interrupted, so this interrupt is deemed to
                    // "belong" to subsequent execution.
                    Thread.currentThread().interrupt();
                    //这种捕获了InterruptException之后调用Thread.currentThread().interrupt()是一种通用的方式。其实就是为了保存中断状态，从而让其他更高层次的代码注意到这个中断。
                }
            }
            //如果屏障被破坏，当前线程抛BrokenBarrierException
            if (g.broken)
                throw new BrokenBarrierException();

            //如果已经换代，直接返回index（last thread已经执行的nextGeneration，但当前线程还没有执行到该语句）
            if (g != generation)
                return index;

            //超时，进行屏障破坏处理，并抛TimeoutException
            if (timed && nanos <= 0L) {
                breakBarrier();
                throw new TimeoutException();
            }
        }
    } finally {
        lock.unlock();    //释放锁
    }
}

//将当前屏障置为破坏状态、重置count、并唤醒所有被阻塞的线程。
//必须先获取锁，才能调用此方法
private void breakBarrier() {
    generation.broken = true;
    count = parties;
    trip.signalAll();
}

//唤醒trip上等待的所有线程，设置下一个Generation
private void nextGeneration() {
    // signal completion of last generation
    trip.signalAll();
    // set up next generation
    count = parties;
    generation = new Generation();
}
```

reset()

```
//重置屏障，先进行屏障破坏处理，再设置下一代generation
public void reset() {
    final ReentrantLock lock = this.lock;
    lock.lock();
    try {
        breakBarrier();   // break the current generation
        nextGeneration(); // start a new generation
    } finally {
        lock.unlock();
    }
}
```

CyclicBarrier与CountDownLatch比较

1）CountDownLatch:一个线程(或者多个)，等待另外N个线程完成某个事情之后才能执行；CyclicBarrier:N个线程相互等待，任何一个线程完成之前，所有的线程都必须等待。

2）CountDownLatch:一次性的；CyclicBarrier:可以重复使用。

3）CountDownLatch基于AQS；CyclicBarrier基于锁和Condition。本质上都是依赖于volatile和CAS实现的。
