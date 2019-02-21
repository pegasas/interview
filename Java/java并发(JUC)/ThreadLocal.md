
# ThreadLocal

ThreadLocal用于保存某个线程共享变量

对于同一个static ThreadLocal，不同线程只能从中get，set，remove自己的变量，而不会影响其他线程的变量。
1、ThreadLocal.get: 获取ThreadLocal中当前线程共享变量的值。
2、ThreadLocal.set: 设置ThreadLocal中当前线程共享变量的值。
3、ThreadLocal.remove: 移除ThreadLocal中当前线程共享变量的值。
4、ThreadLocal.initialValue: ThreadLocal没有被当前线程赋值时或当前线程刚调用remove方法后调用get方法，返回此方法值。
用处：保存线程的独立变量。对一个线程类（继承自Thread)

当使用ThreadLocal维护变量时，ThreadLocal为每个使用该变量的线程提供独立的变量副本，所以每一个线程都可以独立地改变自己的副本，而不会影响其它线程所对应的副本。常用于用户登录控制，如记录session信息。



```
public class ThreadLocalTest {
	public static void main( String[] args ) throws InterruptedException
	{
		final ThreadLocalTest test = new ThreadLocalTest();

		test.set();
		System.out.println( test.getLong() );
		System.out.println( test.getString() );
		/* 在这里新建了一个线程 */
		Thread thread1 = new Thread()
		{
			public void run()
			{
				test.set(); /* 当这里调用了set方法，进一步调用了ThreadLocal的set方法是，会将ThreadLocal变量存储到该线程的ThreadLocalMap类型的成员变量threadLocals中，注意的是这个threadLocals变量是Thread线程的一个变量，而不是ThreadLocal类的变量。 */
				System.out.println( test.getLong() );
				System.out.println( test.getString() );
			};
		};
		thread1.start();
		thread1.join();

		System.out.println( test.getLong() );
		System.out.println( test.getString() );
	}


	ThreadLocal<Long>	longLocal	= new ThreadLocal<Long>();
	ThreadLocal<String>	stringLocal	= new ThreadLocal<String>();

	public void set()
	{
		longLocal.set( Thread.currentThread().getId() );
		stringLocal.set( Thread.currentThread().getName() );
	}


	public long getLong()
	{
		return(longLocal.get() );
	}


	public String getString()
	{
		return(stringLocal.get() );
	}
}
```

## ThreadLocalMap原理

ThreadLocalMap是ThreadLocal的内部类，没有实现Map接口，用独立的方式实现了Map的功能，其内部的Entry也独立实现。

![avatar][1]

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

Hash冲突怎么解决
和HashMap的最大的不同在于，ThreadLocalMap结构非常简单，没有next引用，也就是说ThreadLocalMap中解决Hash冲突的方式并非链表的方式，而是采用线性探测的方式，所谓线性探测，就是根据初始key的hashcode值确定元素在table数组中的位置，如果发现这个位置上已经有其他key值的元素被占用，则利用固定的算法寻找一定步长的下个位置，依次判断，直至找到能够存放的位置。

ThreadLocalMap解决Hash冲突的方式就是简单的步长加1或减1，寻找下一个相邻的位置。

```
/**
 * Increment i modulo len.
 */
private static int nextIndex(int i, int len) {
    return ((i + 1 < len) ? i + 1 : 0);
}

/**
 * Decrement i modulo len.
 */
private static int prevIndex(int i, int len) {
    return ((i - 1 >= 0) ? i - 1 : len - 1);
}
```

显然ThreadLocalMap采用线性探测的方式解决Hash冲突的效率很低，如果有大量不同的ThreadLocal对象放入map中时发送冲突，或者发生二次冲突，则效率很低。

所以这里引出的良好建议是：每个线程只存一个变量，这样的话所有的线程存放到map中的Key都是相同的ThreadLocal，如果一个线程要保存多个变量，就需要创建多个ThreadLocal，多个ThreadLocal放入Map中时会极大的增加Hash冲突的可能。

ThreadLocalMap的问题
由于ThreadLocalMap的key是弱引用，而Value是强引用。这就导致了一个问题，ThreadLocal在没有外部对象强引用时，发生GC时弱引用Key会被回收，而Value不会回收，如果创建ThreadLocal的线程一直持续运行，那么这个Entry对象中的value就有可能一直得不到回收，发生内存泄露。

如何避免泄漏
既然Key是弱引用，那么我们要做的事，就是在调用ThreadLocal的get()、set()方法时完成后再调用remove方法，将Entry节点和Map的引用关系移除，这样整个Entry对象在GC Roots分析后就变成不可达了，下次GC的时候就可以被回收。

如果使用ThreadLocal的set方法之后，没有显示的调用remove方法，就有可能发生内存泄露，所以养成良好的编程习惯十分重要，使用完ThreadLocal之后，记得调用remove方法。

```
ThreadLocal<Session> threadLocal = new ThreadLocal<Session>();
try {
    threadLocal.set(new Session(1, "Misout的博客"));
    // 其它业务逻辑
} finally {
    threadLocal.remove();
}
```

总结

每个ThreadLocal只能保存一个变量副本，如果想要上线一个线程能够保存多个副本以上，就需要创建多个ThreadLocal。

ThreadLocal内部的ThreadLocalMap键为弱引用，会有内存泄漏的风险。

适用于无状态，副本变量独立后不影响业务逻辑的高并发场景。如果如果业务逻辑强依赖于副本变量，则不适合用ThreadLocal解决，需要另寻解决方案。

## ThreadLocal方法源码

get()方法

```
/**
 * Returns the value in the current thread's copy of this
 * thread-local variable.  If the variable has no value for the
 * current thread, it is first initialized to the value returned
 * by an invocation of the {@link #initialValue} method.
 *
 * @return the current thread's value of this thread-local
 */
public T get() {
    Thread t = Thread.currentThread();
    ThreadLocalMap map = getMap(t);
    if (map != null) {
        ThreadLocalMap.Entry e = map.getEntry(this);
        if (e != null)
            return (T)e.value;
    }
    return setInitialValue();
}

ThreadLocalMap getMap(Thread t) {
    return t.threadLocals;
}

private T setInitialValue() {
    T value = initialValue();
    Thread t = Thread.currentThread();
    ThreadLocalMap map = getMap(t);
    if (map != null)
        map.set(this, value);
    else
        createMap(t, value);
    return value;
}

protected T initialValue() {
    return null;
}
```

步骤：
1.获取当前线程的ThreadLocalMap对象threadLocals
2.从map中获取线程存储的K-V Entry节点。
3.从Entry节点获取存储的Value副本值返回。
4.map为空的话返回初始值null，即线程变量副本为null，在使用时需要注意判断NullPointerException。

set()方法

```
/**
 * Sets the current thread's copy of this thread-local variable
 * to the specified value.  Most subclasses will have no need to
 * override this method, relying solely on the {@link #initialValue}
 * method to set the values of thread-locals.
 *
 * @param value the value to be stored in the current thread's copy of
 *        this thread-local.
 */
public void set(T value) {
    Thread t = Thread.currentThread();
    ThreadLocalMap map = getMap(t);
    if (map != null)
        map.set(this, value);
    else
        createMap(t, value);
}

ThreadLocalMap getMap(Thread t) {
    return t.threadLocals;
}

void createMap(Thread t, T firstValue) {
    t.threadLocals = new ThreadLocalMap(this, firstValue);
}
```

步骤：
1.获取当前线程的成员变量map
2.map非空，则重新将ThreadLocal和新的value副本放入到map中。
3.map空，则对线程的成员变量ThreadLocalMap进行初始化创建，并将ThreadLocal和value副本放入map中。

remove()方法

```
/**
 * Removes the current thread's value for this thread-local
 * variable.  If this thread-local variable is subsequently
 * {@linkplain #get read} by the current thread, its value will be
 * reinitialized by invoking its {@link #initialValue} method,
 * unless its value is {@linkplain #set set} by the current thread
 * in the interim.  This may result in multiple invocations of the
 * <tt>initialValue</tt> method in the current thread.
 *
 * @since 1.5
 */
public void remove() {
 ThreadLocalMap m = getMap(Thread.currentThread());
 if (m != null)
     m.remove(this);
}

ThreadLocalMap getMap(Thread t) {
    return t.threadLocals;
}
```

remove方法比较简单，不做赘述。



[1]:data:image/png;base64,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
