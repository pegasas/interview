# wait(), notify(), notifyAll()

一、wait(), notify(), notifyAll()等方法介绍
1.wait()的作用是让当前线程进入等待状态，同时，wait()也会让当前线程释放它所持有的锁。“直到其他线程调用此对象的 notify() 方法或 notifyAll() 方法”，当前线程被唤醒(进入“就绪状态”)

2.notify()和notifyAll()的作用，则是唤醒当前对象上的等待线程；notify()是唤醒单个线程，而notifyAll()是唤醒所有的线程。

3.wait(long timeout)让当前线程处于“等待(阻塞)状态”，“直到其他线程调用此对象的notify()方法或 notifyAll() 方法，或者超过指定的时间量”，当前线程被唤醒(进入“就绪状态”)。

二、wait 的用法详解（这里的t1是一个线程(锁))

```
//     main（主线程）

synchronized(t1) {

       try {

              t1.start();

              t1.wait();

       } catch(InterruptedException e) {

              e.printStackTrace();

       }

}

//     在 t1 线程中唤醒主线程

synchronized (this) {          //这里的 this 为 t1

              this.notify();

}
```

1. synchronized(t1)锁定t1（获得t1的监视器）

2. synchronized(t1)这里的锁定了t1，那么wait需用t1.wait()（释放掉t1）

3. 因为wait需释放锁，所以必须在synchronized中使用（没有锁定则么可以释放？没有锁时使用会抛出IllegalMonitorStateException（正在等待的对象没有锁））

4. notify也要在synchronized使用，应该指定对象，t1. notify()，通知t1对象的等待池里的线程使一个线程进入锁定池，然后与锁定池中的线程争夺锁。那么为什么要在synchronized使用呢？ t1. notify()需要通知一个等待池中的线程，那么这时我们必须得获得t1的监视器（需要使用synchronized），才能对其操作，t1. notify()程序只是知道要对t1操作，但是是否可以操作与是否可以获得t1锁关联的监视器有关。

5. synchronized(),wait,notify() 对象一致性

6. 在while循环里而不是if语句下使用wait（防止虚假唤醒spurious wakeup）
