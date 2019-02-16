# 线程安全Hashmap

为什么线程不安全
个人觉得HashMap在并发时可能出现的问题主要是两方面,首先如果多个线程同时使用put方法添加元素，而且假设正好存在两个put的key发生了碰撞(hash值一样)，那么根据HashMap的实现，这两个key会添加到数组的同一个位置，这样最终就会发生其中一个线程的put的数据被覆盖。第二就是如果多个线程同时检测到元素个数超过数组大小*loadFactor，这样就会发生多个线程同时对Node数组进行扩容，都在重新计算元素位置以及复制数据，但是最终只有一个线程扩容后的数组会赋给table，也就是说其他线程的都会丢失，并且各自线程put的数据也丢失。

如何线程安全的使用HashMap
了解了HashMap为什么线程不安全，那现在看看如何线程安全的使用HashMap。这个无非就是以下三种方式：

Hashtable
ConcurrentHashMap
Synchronized Map

```
//Hashtable
Map<String, String> hashtable = new Hashtable<>();

//synchronizedMap
Map<String, String> synchronizedHashMap = Collections.synchronizedMap(new HashMap<String, String>());

//ConcurrentHashMap
Map<String, String> concurrentHashMap = new ConcurrentHashMap<>();
```

Hashtable

HashTable源码中是使用synchronized来保证线程安全的，比如下面的get方法和put方法：

```
public synchronized V get(Object key) {
    // 省略实现
}
public synchronized V put(K key, V value) {
    // 省略实现
}
```

所以当一个线程访问HashTable的同步方法时，其他线程如果也要访问同步方法，会被阻塞住。

ConcurrentHashMap

SynchronizedMap
看了一下源码，SynchronizedMap的实现还是很简单的。

```
// synchronizedMap方法
public static <K,V> Map<K,V> synchronizedMap(Map<K,V> m) {
       return new SynchronizedMap<>(m);
   }
// SynchronizedMap类
private static class SynchronizedMap<K,V>
       implements Map<K,V>, Serializable {
       private static final long serialVersionUID = 1978198479659022715L;

       private final Map<K,V> m;     // Backing Map
       final Object      mutex;        // Object on which to synchronize

       SynchronizedMap(Map<K,V> m) {
           this.m = Objects.requireNonNull(m);
           mutex = this;
       }

       SynchronizedMap(Map<K,V> m, Object mutex) {
           this.m = m;
           this.mutex = mutex;
       }

       public int size() {
           synchronized (mutex) {return m.size();}
       }
       public boolean isEmpty() {
           synchronized (mutex) {return m.isEmpty();}
       }
       public boolean containsKey(Object key) {
           synchronized (mutex) {return m.containsKey(key);}
       }
       public boolean containsValue(Object value) {
           synchronized (mutex) {return m.containsValue(value);}
       }
       public V get(Object key) {
           synchronized (mutex) {return m.get(key);}
       }

       public V put(K key, V value) {
           synchronized (mutex) {return m.put(key, value);}
       }
       public V remove(Object key) {
           synchronized (mutex) {return m.remove(key);}
       }
       // 省略其他方法
   }
```

从源码中可以看出调用synchronizedMap()方法后会返回一个SynchronizedMap类的对象，而在SynchronizedMap类中使用了synchronized同步关键字来保证对Map的操作是线程安全的。

性能对比
这是要靠数据说话的时代，所以不能只靠嘴说CHM快，它就快了。写个测试用例，实际的比较一下这三种方式的效率(源码来源)，下面的代码分别通过三种方式创建Map对象，使用ExecutorService来并发运行5个线程，每个线程添加/获取500K个元素。

```
public class CrunchifyConcurrentHashMapVsSynchronizedMap {

    public final static int THREAD_POOL_SIZE = 5;

    public static Map<String, Integer> crunchifyHashTableObject = null;
    public static Map<String, Integer> crunchifySynchronizedMapObject = null;
    public static Map<String, Integer> crunchifyConcurrentHashMapObject = null;

    public static void main(String[] args) throws InterruptedException {

        // Test with Hashtable Object
        crunchifyHashTableObject = new Hashtable<>();
        crunchifyPerformTest(crunchifyHashTableObject);

        // Test with synchronizedMap Object
        crunchifySynchronizedMapObject = Collections.synchronizedMap(new HashMap<String, Integer>());
        crunchifyPerformTest(crunchifySynchronizedMapObject);

        // Test with ConcurrentHashMap Object
        crunchifyConcurrentHashMapObject = new ConcurrentHashMap<>();
        crunchifyPerformTest(crunchifyConcurrentHashMapObject);

    }

    public static void crunchifyPerformTest(final Map<String, Integer> crunchifyThreads) throws InterruptedException {

        System.out.println("Test started for: " + crunchifyThreads.getClass());
        long averageTime = 0;
        for (int i = 0; i < 5; i++) {

            long startTime = System.nanoTime();
            ExecutorService crunchifyExServer = Executors.newFixedThreadPool(THREAD_POOL_SIZE);

            for (int j = 0; j < THREAD_POOL_SIZE; j++) {
                crunchifyExServer.execute(new Runnable() {
                    @SuppressWarnings("unused")
                    @Override
                    public void run() {

                        for (int i = 0; i < 500000; i++) {
                            Integer crunchifyRandomNumber = (int) Math.ceil(Math.random() * 550000);

                            // Retrieve value. We are not using it anywhere
                            Integer crunchifyValue = crunchifyThreads.get(String.valueOf(crunchifyRandomNumber));

                            // Put value
                            crunchifyThreads.put(String.valueOf(crunchifyRandomNumber), crunchifyRandomNumber);
                        }
                    }
                });
            }

            // Make sure executor stops
            crunchifyExServer.shutdown();

            // Blocks until all tasks have completed execution after a shutdown request
            crunchifyExServer.awaitTermination(Long.MAX_VALUE, TimeUnit.DAYS);

            long entTime = System.nanoTime();
            long totalTime = (entTime - startTime) / 1000000L;
            averageTime += totalTime;
            System.out.println("2500K entried added/retrieved in " + totalTime + " ms");
        }
        System.out.println("For " + crunchifyThreads.getClass() + " the average time is " + averageTime / 5 + " ms\n");
    }
}
```

测试结果：

```
Test started for: class java.util.Hashtable
2500K entried added/retrieved in 2018 ms
2500K entried added/retrieved in 1746 ms
2500K entried added/retrieved in 1806 ms
2500K entried added/retrieved in 1801 ms
2500K entried added/retrieved in 1804 ms
For class java.util.Hashtable the average time is 1835 ms

Test started for: class java.util.Collections$SynchronizedMap
2500K entried added/retrieved in 3041 ms
2500K entried added/retrieved in 1690 ms
2500K entried added/retrieved in 1740 ms
2500K entried added/retrieved in 1649 ms
2500K entried added/retrieved in 1696 ms
For class java.util.Collections$SynchronizedMap the average time is 1963 ms

Test started for: class java.util.concurrent.ConcurrentHashMap
2500K entried added/retrieved in 738 ms
2500K entried added/retrieved in 696 ms
2500K entried added/retrieved in 548 ms
2500K entried added/retrieved in 1447 ms
2500K entried added/retrieved in 531 ms
For class java.util.concurrent.ConcurrentHashMap the average time is 792 ms
```
