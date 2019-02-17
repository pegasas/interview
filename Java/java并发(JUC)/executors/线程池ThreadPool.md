# java线程池

public static ExecutorService newFixedThreadPool(int nThreads)

创建固定数目线程的线程池，每当提交一个任务时就创建一个线程，直到达到线程池的最大数量，这时线程池的规模将不会再变化(如果某个线程由于发生的Exception而结束，那么线程池会补充一个新的线程)

public static ExecutorService newCachedThreadPool()

创建一个可缓存的线程池，调用execute 将重用以前构造的线程（如果线程可用）。如果现有线程没有可用的，则创建一个新线程并添加到池中。终止并从缓存中移除那些已有 60 秒钟未被使用的线程。

public static ExecutorService newSingleThreadExecutor()

创建一个单线程化的Executor。

public static ScheduledExecutorService newScheduledThreadPool(int corePoolSize)

创建一个固定长度的线程池，而且以延迟或定时的方式来执行任务，类似于Timer。

# 可缓存线程池newCachedThreadPool

```
public class ThreadPoolDemo {
	public static void main (String[] args) {
		ExecutorService cachedThreadPool = Executors.newCachedThreadPool();
		for(int i = 0; i < 10; i++) {
			final int index = i;
			try {
				Thread.sleep(index*1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			cachedThreadPool.execute(new Runnable () {
				public void run () {
					System.out.println(index);
				}
			});
		}
	}

}

```

# 固定长度的线程池newFixedThreadPool

```
public class ThreadPoolDemo {
	public static void main (String[] args) {
		ExecutorService fixedThreadPool = Executors.newFixedThreadPool(3);
		for(int i = 0; i < 10; i++) {
			final int index = i;
			fixedThreadPool.execute(new Runnable () {
				public void run () {
					System.out.println(index);
					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			});
		}
	}
}

```

# 固定长度的线程池，而且以延迟或定时的方式来执行任务

```
public class ThreadPoolDemo {
	public static void main (String[] args) {
		ScheduledExecutorService scheduleThreadPool = Executors.newScheduledThreadPool(5);
		scheduleThreadPool.schedule(new Runnable() {
			public void run() {
				System.out.println("delay 3 seconds");
			}
		}, 3, TimeUnit.SECONDS);
	}
}
```

# 单线程化线程池

```
public class ThreadPoolDemo {
	public static void main (String[] args) {
		ExecutorService singleThreadExecutor = Executors.newSingleThreadExecutor();
		for(int i = 0; i < 10; i++) {
			final int index = i;
			singleThreadExecutor.execute(new Runnable() {
				public void run() {
					try {
						System.out.println(index);
						Thread.sleep(2000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			});
		}
	}
}
```

# 线程池实现

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

任务缓存队列

在前面我们多次提到了任务缓存队列，即workQueue，它用来存放等待执行的任务。

workQueue的类型为BlockingQueue<Runnable>，通常可以取下面三种类型：

1）有界任务队列ArrayBlockingQueue：基于数组的先进先出队列，此队列创建时必须指定大小；

2）无界任务队列LinkedBlockingQueue：基于链表的先进先出队列，如果创建时没有指定此队列大小，则默认为Integer.MAX_VALUE；

3）直接提交队列synchronousQueue：这个队列比较特殊，它不会保存提交的任务，而是将直接新建一个线程来执行新来的任务。

拒绝策略

AbortPolicy:丢弃任务并抛出RejectedExecutionException

CallerRunsPolicy：只要线程池未关闭，该策略直接在调用者线程中，运行当前被丢弃的任务。显然这样做不会真的丢弃任务，但是，任务提交线程的性能极有可能会急剧下降。

DiscardOldestPolicy：丢弃队列中最老的一个请求，也就是即将被执行的一个任务，并尝试再次提交当前任务。

DiscardPolicy：丢弃任务，不做任何处理。

线程池的任务处理策略：

如果当前线程池中的线程数目小于corePoolSize，则每来一个任务，就会创建一个线程去执行这个任务；

如果当前线程池中的线程数目>=corePoolSize，则每来一个任务，会尝试将其添加到任务缓存队列当中，若添加成功，则该任务会等待空闲线程将其取出去执行；若添加失败（一般来说是任务缓存队列已满），则会尝试创建新的线程去执行这个任务；如果当前线程池中的线程数目达到maximumPoolSize，则会采取任务拒绝策略进行处理；

如果线程池中的线程数量大于 corePoolSize时，如果某线程空闲时间超过keepAliveTime，线程将被终止，直至线程池中的线程数目不大于corePoolSize；如果允许为核心池中的线程设置存活时间，那么核心池中的线程空闲时间超过keepAliveTime，线程也会被终止。

线程池的关闭

ThreadPoolExecutor提供了两个方法，用于线程池的关闭，分别是shutdown()和shutdownNow()，其中：

shutdown()：不会立即终止线程池，而是要等所有任务缓存队列中的任务都执行完后才终止，但再也不会接受新的任务

shutdownNow()：立即终止线程池，并尝试打断正在执行的任务，并且清空任务缓存队列，返回尚未执行的任务


# 核心execute方法

```
public void execute(Runnable command) {
        if (command == null)
            throw new NullPointerException();
        /*
         * Proceed in 3 steps:
         *
         * 1. If fewer than corePoolSize threads are running, try to
         * start a new thread with the given command as its first
         * task.  The call to addWorker atomically checks runState and
         * workerCount, and so prevents false alarms that would add
         * threads when it shouldn't, by returning false.
         * 如果正在运行的线程数小于corePoolSize，那么将调用addWorker 方法来创建一个新的线程，并将该任务作为新线程的第一个任务来执行。
　　　　　　 当然，在创建线程之前会做原子性质的检查，如果条件不允许，则不创建线程来执行任务，并返回false.　　

         * 2. If a task can be successfully queued, then we still need
         * to double-check whether we should have added a thread
         * (because existing ones died since last checking) or that
         * the pool shut down since entry into this method. So we
         * recheck state and if necessary roll back the enqueuing if
         * stopped, or start a new thread if there are none.
         * 如果一个任务成功进入阻塞队列，那么我们需要进行一个双重检查来确保是我们已经添加一个线程（因为存在着一些线程在上次检查后他已经死亡）或者
　　　　　　 当我们进入该方法时，该线程池已经关闭。所以，我们将重新检查状态，线程池关闭的情况下则回滚入队列，线程池没有线程的情况则创建一个新的线程。
         * 3. If we cannot queue task, then we try to add a new
         * thread.  If it fails, we know we are shut down or saturated
         * and so reject the task.
　　　　　　 如果任务无法入队列（队列满了），那么我们将尝试新开启一个线程（从corepoolsize到扩充到maximum），如果失败了，那么可以确定原因，要么是
　　　　　　 线程池关闭了或者饱和了（达到maximum），所以我们执行拒绝策略。

         */
　　　　
　　　　// 1.当前线程数量小于corePoolSize，则创建并启动线程。
        int c = ctl.get();
        if (workerCountOf(c) < corePoolSize) {
            if (addWorker(command, true))
　　　　　　　　// 成功，则返回

return;
            c = ctl.get();
        }
　　　　// 2.步骤1失败，则尝试进入阻塞队列，
        if (isRunning(c) && workQueue.offer(command)) {
　　　　　　　// 入队列成功，检查线程池状态，如果状态部署RUNNING而且remove成功，则拒绝任务
            int recheck = ctl.get();
            if (! isRunning(recheck) && remove(command))
                reject(command);
　　　　　　　// 如果当前worker数量为0，通过addWorker(null, false)创建一个线程，其任务为null
            else if (workerCountOf(recheck) == 0)
                addWorker(null, false);
        }
　　　　// 3. 步骤1和2失败，则尝试将线程池的数量有corePoolSize扩充至maxPoolSize，如果失败，则拒绝任务
        else if (!addWorker(command, false))
            reject(command);
    }
```

流程图：
![avatar][1]

结合上面的流程图来逐行解析，首先前面进行空指针检查，

wonrkerCountOf()方法能够取得当前线程池中的线程的总数，取得当前线程数与核心池大小比较，

如果小于，将通过addWorker()方法调度执行。
如果大于核心池大小，那么就提交到等待队列。
如果进入等待队列失败，则会将任务直接提交给线程池。
如果线程数达到最大线程数，那么就提交失败，执行拒绝策略。

# addWorker()

execute()方法中添加任务的方式是使用addWorker()方法

```
private boolean addWorker(Runnable firstTask, boolean core) {
        retry:
　　　　 // 外层循环，用于判断线程池状态
        for (;;) {
            int c = ctl.get();
            int rs = runStateOf(c);

            // Check if queue empty only if necessary.
            if (rs >= SHUTDOWN &&
                ! (rs == SHUTDOWN &&
                   firstTask == null &&
                   ! workQueue.isEmpty()))
                return false;
　　　　　　 // 内层的循环，任务是将worker数量加1
            for (;;) {
                int wc = workerCountOf(c);
                if (wc >= CAPACITY ||
                    wc >= (core ? corePoolSize : maximumPoolSize))
                    return false;
                if (compareAndIncrementWorkerCount(c))
                    break retry;
                c = ctl.get();  // Re-read ctl
                if (runStateOf(c) != rs)
                    continue retry;
                // else CAS failed due to workerCount change; retry inner loop
            }
        }
　　　　// worker加1后，接下来将woker添加到HashSet<Worker>中，并启动worker
        boolean workerStarted = false;
        boolean workerAdded = false;
        Worker w = null;
        try {
            final ReentrantLock mainLock = this.mainLock;
            w = new Worker(firstTask);
            final Thread t = w.thread;
            if (t != null) {
                mainLock.lock();
                try {
                    // Recheck while holding lock.
                    // Back out on ThreadFactory failure or if
                    // shut down before lock acquired.
                    int c = ctl.get();
                    int rs = runStateOf(c);

                    if (rs < SHUTDOWN ||
                        (rs == SHUTDOWN && firstTask == null)) {
                        if (t.isAlive()) // precheck that t is startable
                            throw new IllegalThreadStateException();
                        workers.add(w);
                        int s = workers.size();
                        if (s > largestPoolSize)
                            largestPoolSize = s;
                        workerAdded = true;
                    }
                } finally {
                    mainLock.unlock();
                }
　　　　　　　　　// 如果往HashSet<Worker>添加成功，则启动该线程
                if (workerAdded) {
                    t.start();
                    workerStarted = true;
                }
            }
        } finally {
            if (! workerStarted)
                addWorkerFailed(w);
        }
        return workerStarted;
    }
```

addWorker(Runnable firstTask, boolean core)的主要任务是创建并启动线程。

他会根据当前线程的状态和给定的值（core or maximum）来判断是否可以创建一个线程。

addWorker共有四种传参方式。execute使用了其中三种，分别为:

1.addWorker(paramRunnable, true)

线程数小于corePoolSize时，放一个需要处理的task进Workers Set。如果Workers Set长度超过corePoolSize，就返回false.

2.addWorker(null, false)

放入一个空的task进workers Set，长度限制是maximumPoolSize。这样一个task为空的worker在线程执行的时候会去任务队列里拿任务，这样就相当于创建了一个新的线程，只是没有马上分配任务。

3.addWorker(paramRunnable, false)

当队列被放满时，就尝试将这个新来的task直接放入Workers Set，而此时Workers Set的长度限制是maximumPoolSize。如果线程池也满了的话就返回false.

还有一种情况是execute()方法没有使用的

addWorker(null, true)

这个方法就是放一个null的task进Workers Set，而且是在小于corePoolSize时，如果此时Set中的数量已经达到corePoolSize那就返回false，什么也不干。实际使用中是在prestartAllCoreThreads()方法，这个方法用来为线程池预先启动corePoolSize个worker等待从workQueue中获取任务执行。

执行流程：

1、判断线程池当前是否为可以添加worker线程的状态，可以则继续下一步，不可以return false：
    A、线程池状态>shutdown，可能为stop、tidying、terminated，不能添加worker线程
    B、线程池状态==shutdown，firstTask不为空，不能添加worker线程，因为shutdown状态的线程池不接收新任务
    C、线程池状态==shutdown，firstTask==null，workQueue为空，不能添加worker线程，因为firstTask为空是为了添加一个没有任务的线程再从workQueue获取task，而workQueue为  　　　　空，说明添加无任务线程已经没有意义
2、线程池当前线程数量是否超过上限（corePoolSize 或 maximumPoolSize），超过了return false，没超过则对workerCount+1，继续下一步
3、在线程池的ReentrantLock保证下，向Workers Set中添加新创建的worker实例，添加完成后解锁，并启动worker线程，如果这一切都成功了，return true，如果添加worker入Set失败或启动失败，调用addWorkerFailed()逻辑

如何选择线程池数量
线程池的大小决定着系统的性能，过大或者过小的线程池数量都无法发挥最优的系统性能。

当然线程池的大小也不需要做的太过于精确，只需要避免过大和过小的情况。一般来说，确定线程池的大小需要考虑CPU的数量，内存大小，任务是计算密集型还是IO密集型等因素

NCPU = CPU的数量

UCPU = 期望对CPU的使用率 0 ≤ UCPU ≤ 1

W/C = 等待时间与计算时间的比率

如果希望处理器达到理想的使用率，那么线程池的最优大小为：

线程池大小=NCPU *UCPU(1+W/C)

[1]:data:image/png;base64,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
