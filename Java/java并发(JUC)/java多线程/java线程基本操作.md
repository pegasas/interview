# java线程基本操作

## 新建线程

新建线程用new新建一个Thread对象，然后将其start即可

```
Thread t = new Thread();
t.start();
```

start()和run()的区别:

调用start方法方可启动线程，而run方法只是thread的一个普通方法调用，还是在主线程里执行。

start():

使该线程开始执行；Java 虚拟机调用该线程的 run 方法。

结果是两个线程并发地运行；当前线程（从调用返回给 start 方法）和另一个线程（执行其 run 方法）。

多次启动一个线程是非法的。特别是当线程已经结束执行后，不能再重新启动。

用start方法来启动线程，真正实现了多线程运行，这时无需等待run方法体代码执行完毕而直接继续执行下面的代码。通过调用Thread类的 start()方法来启动一个线程，这时此线程处于就绪（可运行）状态，并没有运行，一旦得到cpu时间片，就开始执行run()方法，这里方法 run()称为线程体，它包含了要执行的这个线程的内容，Run方法运行结束，此线程随即终止。

run():

我们还是先看看API中对该方法的介绍：

如果该线程是使用独立的 Runnable 运行对象构造的，则调用该 Runnable 对象的 run 方法；否则，该方法不执行任何操作并返回。

Thread 的子类应该重写该方法。

run()方法只是类的一个普通方法而已，如果直接调用Run方法，程序中依然只有主线程这一个线程，其程序执行路径还是只有一条，还是要顺序执行，还是要等待run方法体执行完毕后才可继续执行下面的代码，这样就没有达到写线程的目的。

## 终止线程

有2种方法可以终止线程：

1.设置退出标志，使线程正常退出，也就是当run()方法完成后线程终止
3.使用stop方法强行终止线程（不推荐使用，Thread.stop, Thread.suspend, Thread.resume 和Runtime.runFinalizersOnExit 这些终止线程运行的方法已经被废弃，使用它们是极端不安全的！）

1.使用退出标志终止线程

一般run()方法执行完，线程就会正常结束，然而，常常有些线程是伺服线程。它们需要长时间的运行，只有在外部某些条件满足的情况下，才能关闭这些线程。使用一个变量来控制循环，例如：最直接的方法就是设一个boolean类型的标志，并通过设置这个标志为true或false来控制while循环是否退出，代码示例：

```
public class ThreadSafe extends Thread {
    public volatile boolean exit = false;
        public void run() {
        while (!exit){
            //do something
        }
    }
}
```

定义了一个退出标志exit，当exit为true时，while循环退出，exit的默认值为false.在定义exit时，使用了一个Java关键字volatile，这个关键字的目的是使exit同步，也就是说在同一时刻只能由一个线程来修改exit的值.

2.使用stop方法终止线程
程序中可以直接使用thread.stop()来强行终止线程，但是stop方法是很危险的，就象突然关闭计算机电源，而不是按正常程序关机一样，可能会产生不可预料的结果，不安全主要是：thread.stop()调用之后，创建子线程的线程就会抛出ThreadDeatherror的错误，并且会释放子线程所持有的所有锁。一般任何进行加锁的代码块，都是为了保护数据的一致性，如果在调用thread.stop()后导致了该线程所持有的所有锁的突然释放(不可控制)，那么被保护数据就有可能呈现不一致性，其他线程在使用这些被破坏的数据时，有可能导致一些很奇怪的应用程序错误。因此，并不推荐使用stop方法来终止线程。

# 线程中断

有3个方法提供对线程中断的支持：
```
public boolean isInterrupted()
public void interrupt()
public static boolean interrupted()
```

public static boolean interrupted()测试当前线程是否已经中断。线程的中断状态 由该方法清除。换句话说，如果连续两次调用该方法，则第二次调用将返回 false（在第一次调用已清除了其中断状态之后，且第二次调用检验完中断状态前，当前线程再次中断的情况除外）

public booleanisInterrupted()测试线程是否已经中断。线程的中断状态不受该方法的影响

public voidinterrupt()中断线程

interrupt是一个实例方法，该方法用于设置当前线程对象的中断标识位。

interrupted是一个静态的方法，用于返回当前线程是否被中断。

使用interrupt()方法中断当前线程

使用interrupt()方法来中断线程有两种情况：

1.线程处于阻塞状态，如使用了sleep,同步锁的wait,socket中的receiver,accept等方法时，会使线程处于阻塞状态。当调用线程的interrupt()方法时，会抛出InterruptException异常。阻塞中的那个方法抛出这个异常，通过代码捕获该异常，然后break跳出循环状态，从而让我们有机会结束这个线程的执行。通常很多人认为只要调用interrupt方法线程就会结束，实际上是错的， 一定要先捕获InterruptedException异常之后通过break来跳出循环，才能正常结束run方法。

```
public class ThreadSafe extends Thread {
    public void run() {
        while (true){
            try{
                    Thread.sleep(5*1000);//阻塞5妙
                }catch(InterruptedException e){
                    e.printStackTrace();
                    break;//捕获到异常之后，执行break跳出循环。
                }
        }
    }
}
```

2.线程未处于阻塞状态，使用isInterrupted()判断线程的中断标志来退出循环。当使用interrupt()方法时，中断标志就会置true，和使用自定义的标志来控制循环是一样的道理。

```
public class ThreadSafe extends Thread {
    public void run() {
        while (!isInterrupted()){
            //do something, but no throw InterruptedException
        }
    }
}
```

为什么要区分进入阻塞状态和和非阻塞状态两种情况了，是因为当阻塞状态时，如果有interrupt()发生，系统除了会抛出InterruptedException异常外，还会调用interrupted()函数，调用时能获取到中断状态是true的状态，调用完之后会复位中断状态为false，所以异常抛出之后通过isInterrupted()是获取不到中断状态是true的状态，从而不能退出循环，因此在线程未进入阻塞的代码段时是可以通过isInterrupted()来判断中断是否发生来控制循环，在进入阻塞状态后要通过捕获异常来退出循环。因此使用interrupt()来退出线程的最好的方式应该是两种情况都要考虑：

```
public class ThreadSafe extends Thread {
    public void run() {
        while (!isInterrupted()){ //非阻塞过程中通过判断中断标志来退出
            try{
                Thread.sleep(5*1000);//阻塞过程捕获中断异常来退出
            }catch(InterruptedException e){
                e.printStackTrace();
                break;//捕获到异常之后，执行break跳出循环。
            }
        }
    }
}
```

# sleep()

Thread.sleep(long millis)和Thread.sleep(long millis, int nanos)静态方法强制当前正在执行的线程休眠（暂停执行），以“减慢线程”。

当线程睡眠时，它睡在某个地方，在苏醒之前不会返回到可运行状态。

当睡眠时间到期，则返回到可运行状态。

# 等待(wait)和通知(notify)

在java中，线程间的通信可以使用wait、notify、notifyAll来进行控制。从名字就可以看出来这3个方法都是跟多线程相关的，但是可能让你感到吃惊的是：这3个方法并不是Thread类或者是Runnable接口的方法，而是Object类的3个本地方法。

其实要理解这一点也并不难，调用一个Object的wait与notify/notifyAll的时候，必须保证调用代码对该Object是同步的，也就是说必须在作用等同于synchronized(obj){......}的内部才能够去调用obj的wait与notify/notifyAll三个方法，否则就会报错：

也就是说，在调用这3个方法的时候，当前线程必须获得这个对象的锁，那么这3个方法就是和对象锁相关的，所以是属于Object的方法而不是Thread，因为不是每个对象都是Thread。所以我们在理解wait、notify、notifyAll之前，先要了解以下对象锁。

多个线程都持有同一个对象的时候，如果都要进入synchronized(obj){......}的内部，就必须拿到这个对象的锁，synchronized的机制保证了同一时间最多只能有1个线程拿到了对象的锁，如下图：

![avatar][1]

下面我们来看一下这3个方法的作用：
wait：线程自动释放其占有的对象锁，并等待notify
notify：唤醒一个正在wait当前对象锁的线程，并让它拿到对象锁
notifyAll：唤醒所有正在wait前对象锁的线程

notify和notifyAll的最主要的区别是：notify只是唤醒一个正在wait当前对象锁的线程，而notifyAll唤醒所有。值得注意的是：notify是本地方法，具体唤醒哪一个线程由虚拟机控制；notifyAll后并不是所有的线程都能马上往下执行，它们只是跳出了wait状态，接下来它们还会是竞争对象锁。

下面通过一个常用生产者、消费者的例子来说明。
消息实体类：

```
package com.podongfeng;

public class Message {
}
```

生产者：

```
package com.podongfeng;

import java.util.ArrayList;
import java.util.List;

public class Producer extends Thread {

    List<Message> msgList = new ArrayList<>();

    @Override public void run() {
        try {
            while (true) {
                Thread.sleep(3000);
                Message msg = new Message();
                synchronized(msgList) {
                    msgList.add(msg);
                    msgList.notify(); //这里只能是notify而不能是notifyAll，否则remove(0)会报java.lang.IndexOutOfBoundsException: Index: 0, Size: 0
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public Message waitMsg() {
        synchronized(msgList) {
            if(msgList.size() == 0) {
                try {
                    msgList.wait();
                } catch(InterruptedException e) {
                    e.printStackTrace();
                }
            }
            return msgList.remove(0);
        }
    }
}
```

消费者：

```
package com.podongfeng;

public class Consumer extends Thread {

    private Producer producer;

    public Consumer(String name, Producer producer) {
        super(name);
        this.producer = producer;
    }

    @Override public void run() {
        while (true) {
            Message msg = producer.waitMsg();
            System.out.println("Consumer " + getName() + " get a msg");
        }
    }

    public static void main(String[] args) {
        Producer p = new Producer();
        p.start();
        new Consumer("Consumer1", p).start();
        new Consumer("Consumer2", p).start();
        new Consumer("Consumer3", p).start();
    }
}
```

消费者线程调用waitMsg去获取一个消息实体，如果msgList为空，则线程进入wait状态；生产这线程每隔3秒钟生产出体格msg实体并放入msgList列表，完成后，调用notify唤醒一个消费者线程去消费。

最后再次提醒注意：
wait、notify、notifyAll并不是Thread类或者是Runnable接口的方法，而是Object类的3个本地方法。
在调用这3个方法的时候，当前线程必须获得这个对象的锁

# 挂起(suspend)和继续执行(resume)

1、suspend将线程挂起，运行->阻塞；调用后并不释放所占用的锁
resume将线程解挂，阻塞->就绪
2、suspend和resume这两种方法不建议使用，因为存在很多弊端。
（1）独占：因为suspend在调用过程中不会释放所占用的锁，所以如果使用不当会造成对公共对象的独占，使得其他线程无法访问公共对象，严重的话造成死锁。

```
Public void run(){
While(true){
i++;
//System.out.println(i);
}}
 
Public static void main(String[] args){ ...
thread.suspend();
System.out.println(“main end”);
...}
```

首先运行的时候会出现main end；

如果将注释的println语句添加进来，则结果大不相同程序会输出 i=0...i=3333但是挂起后不会输出main end
这是因为println函数，suspend挂起后的线程没有释放锁，导致其他线程在调用println函数时出现阻塞
Public void println(long x){
Synchronized(this){
Print(x);
newLine(); }}

（2）不同步：容易出现因线程暂停导致的数据不同步

# 等待结束(join)和谦让(yield)

注意Java并不限定线程是以时间片运行，但是大多数操作系统却有这样的要求。在术语中经常引起混淆：抢占经常与时间片混淆。事实上，抢占意味着只有拥有高优先级的线程可以优先于低优先级的线程执行，但是当线程拥有相同优先级的时候，他们不能相互抢占。它们通常受时间片管制，但这并不是Java的要求。

## 理解线程的优先权

接下来，理解线程优先级是多线程学习很重要的一步，尤其是了解yield()函数的工作过程。

记住当线程的优先级没有指定时，所有线程都携带普通优先级。
优先级可以用从1到10的范围指定。10表示最高优先级，1表示最低优先级，5是普通优先级。
记住优先级最高的线程在执行时被给予优先。但是不能保证线程在启动时就进入运行状态。
与在线程池中等待运行机会的线程相比，当前正在运行的线程可能总是拥有更高的优先级。
由调度程序决定哪一个线程被执行。
t.setPriority()用来设定线程的优先级。
记住在线程开始方法被调用之前，线程的优先级应该被设定。
你可以使用常量，如MIN_PRIORITY,MAX_PRIORITY，NORM_PRIORITY来设定优先级

## yield()

一个调用yield()方法的线程告诉虚拟机它乐意让其他线程占用自己的位置。这表明该线程没有在做一些紧急的事情。注意，这仅是一个暗示，并不能保证不会产生任何影响。

Yield是一个静态的原生(native)方法
Yield告诉当前正在执行的线程把运行机会交给线程池中拥有相同优先级的线程。
Yield不能保证使得当前正在运行的线程迅速转换到可运行的状态
它仅能使一个线程从运行状态转到可运行状态，而不是等待或阻塞状态

```
package test.core.threads;

public class YieldExample
{
   public static void main(String[] args)
   {
      Thread producer = new Producer();
      Thread consumer = new Consumer();

      producer.setPriority(Thread.MIN_PRIORITY); //Min Priority
      consumer.setPriority(Thread.MAX_PRIORITY); //Max Priority

      producer.start();
      consumer.start();
   }
}

class Producer extends Thread
{
   public void run()
   {
      for (int i = 0; i < 5; i++)
      {
         System.out.println("I am Producer : Produced Item " + i);
         Thread.yield();
      }
   }
}

class Consumer extends Thread
{
   public void run()
   {
      for (int i = 0; i < 5; i++)
      {
         System.out.println("I am Consumer : Consumed Item " + i);
         Thread.yield();
      }
   }
}
```

上述程序在没有调用yield()方法情况下的输出：

```
I am Consumer : Consumed Item 0
I am Consumer : Consumed Item 1
I am Consumer : Consumed Item 2
I am Consumer : Consumed Item 3
I am Consumer : Consumed Item 4
I am Producer : Produced Item 0
I am Producer : Produced Item 1
I am Producer : Produced Item 2
I am Producer : Produced Item 3
I am Producer : Produced Item 4
```

上述程序在调用yield()方法情况下的输出：

```
I am Producer : Produced Item 0
I am Consumer : Consumed Item 0
I am Producer : Produced Item 1
I am Consumer : Consumed Item 1
I am Producer : Produced Item 2
I am Consumer : Consumed Item 2
I am Producer : Produced Item 3
I am Consumer : Consumed Item 3
I am Producer : Produced Item 4
I am Consumer : Consumed Item 4
```

# join()

线程实例的方法join()方法可以使得一个线程在另一个线程结束后再执行。如果join()方法在一个线程实例上调用，当前运行着的线程将阻塞直到这个线程实例完成了执行。

在join()方法内设定超时，使得join()方法的影响在特定超时后无效。当超时时，主方法和任务线程申请运行的时候是平等的。然而，当涉及sleep时，join()方法依靠操作系统计时，所以你不应该假定join()方法将会等待你指定的时间。

像sleep,join通过抛出InterruptedException对中断做出回应。

```
package test.core.threads;

public class JoinExample
{
   public static void main(String[] args) throws InterruptedException
   {
      Thread t = new Thread(new Runnable()
         {
            public void run()
            {
               System.out.println("First task started");
               System.out.println("Sleeping for 2 seconds");
               try
               {
                  Thread.sleep(2000);
               } catch (InterruptedException e)
               {
                  e.printStackTrace();
               }
               System.out.println("First task completed");
            }
         });
      Thread t1 = new Thread(new Runnable()
         {
            public void run()
            {
               System.out.println("Second task completed");
            }
         });
      t.start(); // Line 15
      t.join(); // Line 16
      t1.start();
   }
}

Output:

First task started
Sleeping for 2 seconds
First task completed
Second task completed
```

[1]:data:image/png;base64,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
