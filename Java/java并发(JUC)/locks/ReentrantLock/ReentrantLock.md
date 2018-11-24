# 重入锁 ReentrantLock

重入锁的使用

```
public class ReentrantLockTest implements Runnable {
    public static ReentrantLock lock = new ReentrantLock();
    public static int i = 0;

    @Override
    public void run() {
        for (int j = 0; j < 10000; j++) {
            lock.lock();
            try {
                i++;
            } finally {
                lock.unlock();
            }
        }
    }

    public static void main(String[] args) throws InterruptedException {
        ReentrantLockTest test = new ReentrantLockTest();
        Thread t1 = new Thread(test);
        Thread t2 = new Thread(test);
        t1.start();t2.start();
        t1.join(); t2.join(); // main线程会等待t1和t2都运行完再执行以后的流程
        System.err.println(i);
    }
}
```

# 中断响应

```
public class KillDeadlock implements Runnable{
    public static ReentrantLock lock1 = new ReentrantLock();
    public static ReentrantLock lock2 = new ReentrantLock();
    int lock;

    public KillDeadlock(int lock) {
        this.lock = lock;
    }

    @Override
    public void run() {
        try {
            if (lock == 1) {
                lock1.lockInterruptibly();  // 以可以响应中断的方式加锁
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {}
                lock2.lockInterruptibly();
            } else {
                lock2.lockInterruptibly();  // 以可以响应中断的方式加锁
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {}
                lock1.lockInterruptibly();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            if (lock1.isHeldByCurrentThread()) lock1.unlock();  // 注意判断方式
            if (lock2.isHeldByCurrentThread()) lock2.unlock();
            System.err.println(Thread.currentThread().getId() + "退出！");
        }
    }

    public static void main(String[] args) throws InterruptedException {
        KillDeadlock deadLock1 = new KillDeadlock(1);
        KillDeadlock deadLock2 = new KillDeadlock(2);
        Thread t1 = new Thread(deadLock1);
        Thread t2 = new Thread(deadLock2);
        t1.start();t2.start();
        Thread.sleep(1000);
        t2.interrupt(); // ③
    }
}
```
t1、t2线程开始运行时，会分别持有lock1和lock2而请求lock2和lock1，这样就发生了死锁。但是，在③处给t2线程状态标记为中断后，持有重入锁lock2的线程t2会响应中断，并不再继续等待lock1，同时释放了其原本持有的lock2，这样t1获取到了lock2，正常执行完成。t2也会退出，但只是释放了资源并没有完成工作。

# 锁申请等待限时

可以使用 tryLock()或者tryLock(long timeout, TimeUtil unit) 方法进行一次限时的锁等待。

前者不带参数，这时线程尝试获取锁，如果获取到锁则继续执行，如果锁被其他线程持有，则立即返回 false ，也就是不会使当前线程等待，所以不会产生死锁。
后者带有参数，表示在指定时长内获取到锁则继续执行，如果等待指定时长后还没有获取到锁则返回false。

```
public class TryLockTest implements Runnable{
    public static ReentrantLock lock = new ReentrantLock();

    @Override
    public void run() {
        try {
            if (lock.tryLock(1, TimeUnit.SECONDS)) { // 等待1秒
                Thread.sleep(2000);  //休眠2秒
            } else {
                System.err.println(Thread.currentThread().getName() + "获取锁失败！");
            }
        } catch (Exception e) {
            if (lock.isHeldByCurrentThread()) lock.unlock();
        }
    }

    public static void main(String[] args) throws InterruptedException {
        TryLockTest test = new TryLockTest();
        Thread t1 = new Thread(test); t1.setName("线程1");
        Thread t2 = new Thread(test); t1.setName("线程2");
        t1.start();t2.start();
    }
}
/**
 * 运行结果:
 * 线程2获取锁失败！
 */
```

上述示例中，t1先获取到锁，并休眠2秒，这时t2开始等待，等待1秒后依然没有获取到锁，就不再继续等待，符合预期结果。



3、公平锁

所谓公平锁，就是按照时间先后顺序，使先等待的线程先得到锁，而且，公平锁不会产生饥饿锁，也就是只要排队等待，最终能等待到获取锁的机会。使用重入锁（默认是非公平锁）创建公平锁：

```
public ReentrantLock(boolean fair) {
    sync = fair ? new FairSync() : new NonfairSync();
}
```

```
public class FairLockTest implements Runnable{
    public static ReentrantLock lock = new ReentrantLock(true);

    @Override
    public void run() {
        while (true) {
            try {
                lock.lock();
                System.err.println(Thread.currentThread().getName() + "获取到了锁！");
            } finally {
                lock.unlock();
            }
        }
    }

    public static void main(String[] args) throws InterruptedException {
        FairLockTest test = new FairLockTest();
        Thread t1 = new Thread(test, "线程1");
        Thread t2 = new Thread(test, "线程2");
        t1.start();t2.start();
    }
}
/**
 * 运行结果:
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * 线程1获取到了锁！
 * 线程2获取到了锁！
 * ......（上边是截取的一段）
 */
```

可以发现，t1和t2交替获取到锁。如果是非公平锁，会发生t1运行了许多遍后t2才开始运行的情况。

ReentrantLock几个重要方法总结如下:
```
lock()获得锁，如果锁已经被占用，则等待
lockInterruptibly()获得锁，但优先响应中断
tryLock()尝试获得锁，如果成功返回true，失败返回false，该方法不等待立刻返回
tryLock(long time, TimeUnit unit)在给定时间内尝试获得锁
unlock()释放锁
```

# ReentrantLock 配合 Condition 使用

配合ReentrantLock 使用的Conditon提供了以下方法：

```
public interface Condition {
    void await() throws InterruptedException; // 类似于Object.wait()
    void awaitUninterruptibly(); // 与await()相同，但不会再等待过程中响应中断
    long awaitNanos(long nanosTimeout) throws InterruptedException;
    boolean await(long time, TimeUnit unit) throws InterruptedException;
    boolean awaitUntil(Date deadline) throws InterruptedException;
    void signal(); // 类似于Obejct.notify()
    void signalAll();
}
```

ReentrantLock 实现了Lock接口，可以通过该接口提供的newCondition()方法创建Condition对象：

```
public interface Lock {
    void lock();
    void lockInterruptibly() throws InterruptedException;
    boolean tryLock();
    boolean tryLock(long time, TimeUnit unit) throws InterruptedException;
    void unlock();
    Condition newCondition();
}
```

```
public class ReentrantLockWithConditon implements Runnable{
    public static ReentrantLock lock = new ReentrantLock(true);
    public static Condition condition = lock.newCondition();

    @Override
    public void run() {
        lock.newCondition();
        try {
            lock.lock();
            System.err.println(Thread.currentThread().getName() + "-线程开始等待...");
            condition.await();
            System.err.println(Thread.currentThread().getName() + "-线程继续进行了");
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            lock.unlock();
        }
    }

    public static void main(String[] args) throws InterruptedException {
        ReentrantLockWithConditon test = new ReentrantLockWithConditon();
        Thread t = new Thread(test, "线程ABC");
        t.start();
        Thread.sleep(1000);
        System.err.println("过了1秒后...");
        lock.lock();
        condition.signal(); // 调用该方法前需要获取到创建该对象的锁否则会产生
                            // java.lang.IllegalMonitorStateException异常
        lock.unlock();
    }
}
```

# 读写锁

读-读不互斥：读读之间不阻塞
读-写互斥：读阻塞写，写阻塞读
写-写互斥：写写阻塞

读锁，此时多个线程可以或得读锁

```
public void read()
{
	try {
		try {
			lock.readLock().lock();
			System.out.println( "获得读锁" + Thread.currentThread().getName()
					    + " " + System.currentTimeMillis() );
			Thread.sleep( 10000 );
		} finally {
			lock.readLock().unlock();
		}
	} catch ( InterruptedException e ) {
		/* TODO Auto-generated catch block */
		e.printStackTrace();
	}
}
```

写锁，此时只有一个线程能获得写锁

```
public void write()
{
	try {
		try {
			lock.writeLock().lock();
			System.out.println( "获得写锁" + Thread.currentThread().getName()
					    + " " + System.currentTimeMillis() );
			Thread.sleep( 10000 );
		} finally {
			lock.writeLock().unlock();
		}
	} catch ( InterruptedException e ) {
		e.printStackTrace();
	}
}
```
