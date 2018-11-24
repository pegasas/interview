# AtomInteger

AtomicInteger 的源码

```
public class AtomicInteger extends Number implements java.io.Serializable {
    private static final long serialVersionUID = 6214790243416807050L;

    // setup to use Unsafe.compareAndSwapInt for updates
    private static final Unsafe unsafe = Unsafe.getUnsafe();
    private static final long valueOffset;

    static {
      try {
        valueOffset = unsafe.objectFieldOffset
            (AtomicInteger.class.getDeclaredField("value"));
      } catch (Exception ex) { throw new Error(ex); }
    }

    private volatile int value;

    public AtomicInteger(int initialValue) {
        value = initialValue;
    }

    public AtomicInteger() {
    }

    public final int get() {
        return value;
    }

    public final void set(int newValue) {
        value = newValue;
    }

    public final void lazySet(int newValue) {
        unsafe.putOrderedInt(this, valueOffset, newValue);
    }

    public final int getAndSet(int newValue) {
        for (;;) {
            int current = get();
            if (compareAndSet(current, newValue))
                return current;
        }
    }

    public final boolean compareAndSet(int expect, int update) {
    return unsafe.compareAndSwapInt(this, valueOffset, expect, update);
    }

    public final boolean weakCompareAndSet(int expect, int update) {
    return unsafe.compareAndSwapInt(this, valueOffset, expect, update);
    }

    public final int getAndIncrement() {
        for (;;) {
            int current = get();
            int next = current + 1;
            if (compareAndSet(current, next))
                return current;
        }
    }

    public final int getAndDecrement() {
        for (;;) {
            int current = get();
            int next = current - 1;
            if (compareAndSet(current, next))
                return current;
        }
    }

    public final int getAndAdd(int delta) {
        for (;;) {
            int current = get();
            int next = current + delta;
            if (compareAndSet(current, next))
                return current;
        }
    }

    public final int incrementAndGet() {
        for (;;) {
            int current = get();
            int next = current + 1;
            if (compareAndSet(current, next))
                return next;
        }
    }

    public final int decrementAndGet() {
        for (;;) {
            int current = get();
            int next = current - 1;
            if (compareAndSet(current, next))
                return next;
        }
    }

    public final int addAndGet(int delta) {
        for (;;) {
            int current = get();
            int next = current + delta;
            if (compareAndSet(current, next))
                return next;
        }
    }

    public String toString() {
        return Integer.toString(get());
    }


    public int intValue() {
    return get();
    }

    public long longValue() {
    return (long)get();
    }

    public float floatValue() {
    return (float)get();
    }

    public double doubleValue() {
    return (double)get();
    }

}
```

我们先看原子整型类中定义的属性

```
// setup to use Unsafe.compareAndSwapInt for updates
private static final Unsafe unsafe = Unsafe.getUnsafe();
private static final long valueOffset;

static {
  try {
    valueOffset = unsafe.objectFieldOffset
        (AtomicInteger.class.getDeclaredField("value"));
  } catch (Exception ex) { throw new Error(ex); }
}
```

Unsafe是JDK内部的工具类，主要实现了平台相关的操作。下面内容引自

sun.misc.Unsafe是JDK内部用的工具类。它通过暴露一些Java意义上说“不安全”的功能给Java层代码，来让JDK能够更多的使用Java代码来实现一些原本是平台相关的、需要使用native语言（例如C或C++）才可以实现的功能。该类不应该在JDK核心类库之外使用。

Unsafe的具体实现跟本篇的目标关联不大，你只要知道这段代码是为了获取value在堆内存中的偏移量就够了。偏移量在AtomicInteger中很重要，原子操作都靠它来实现。

Value的定义和volatile

AtomicInteger 本身是个整型，所以最重要的属性就是value，我们看看它是如何声明value的

```
private volatile int value;
```

我们看到value使用了volatile修饰符，那么什么是volatile呢？

volatile相当于synchronized的弱实现，也就是说volatile实现了类似synchronized的语义，却又没有锁机制。它确保对volatile字段的更新以可预见的方式告知其他的线程。

volatile包含以下语义：

Java 存储模型不会对valatile指令的操作进行重排序：这个保证对volatile变量的操作时按照指令的出现顺序执行的。

volatile变量不会被缓存在寄存器中（只有拥有线程可见）或者其他对CPU不可见的地方，每次总是从主存中读取volatile变量的结果。也就是说对于volatile变量的修改，其它线程总是可见的，并且不是使用自己线程栈内部的变量。也就是在happens-before法则中，对一个valatile变量的写操作后，其后的任何读操作理解可见此写操作的结果。

用CAS操作实现安全的自增

AtomicInteger中有很多方法，例如incrementAndGet() 相当于i++ 和getAndAdd() 相当于i+=n 。从源码中我们可以看出这几种方法的实现很相似，所以我们主要分析incrementAndGet() 方法的源码。

```
public final int incrementAndGet() {
    for (;;) {
        int current = get();
        int next = current + 1;
        if (compareAndSet(current, next))
            return next;
    }
}

public final boolean compareAndSet(int expect, int update) {
    return unsafe.compareAndSwapInt(this, valueOffset, expect, update);
}
```

incrementAndGet() 方法实现了自增的操作。核心实现是先获取当前值和目标值（也就是value+1），如果compareAndSet(current, next) 返回成功则该方法返回目标值。那么compareAndSet是做什么的呢？理解这个方法我们需要引入CAS操作。

在大学操作系统课程中我们学过独占锁和乐观锁的概念。独占锁就是线程获取锁后其他的线程都需要挂起，直到持有独占锁的线程释放锁；乐观锁是先假定没有冲突直接进行操作，如果因为有冲突而失败就重试，直到操作成功。其中乐观锁用到的机制就是CAS，Compare and Swap。

AtomicInteger 中的CAS操作就是compareAndSet()，其作用是每次从内存中根据内存偏移量（valueOffset）取出数据，将取出的值跟expect 比较，如果数据一致就把内存中的值改为update。

# AtomicReference 原子引用

AtomicReference函数列表

```
// 使用 null 初始值创建新的 AtomicReference。
AtomicReference()
// 使用给定的初始值创建新的 AtomicReference。
AtomicReference(V initialValue)

// 如果当前值 == 预期值，则以原子方式将该值设置为给定的更新值。
boolean compareAndSet(V expect, V update)
// 获取当前值。
V get()
// 以原子方式设置为给定值，并返回旧值。
V getAndSet(V newValue)
// 最终设置为给定值。
void lazySet(V newValue)
// 设置为给定值。
void set(V newValue)
// 返回当前值的字符串表示形式。
String toString()
// 如果当前值 == 预期值，则以原子方式将该值设置为给定的更新值。
boolean weakCompareAndSet(V expect, V update)
```

AtomicReference源码分析(基于JDK1.7.0_40)
说明：
AtomicReference的源码比较简单。它是通过"volatile"和"Unsafe提供的CAS函数实现"原子操作。
(01) value是volatile类型。这保证了：当某线程修改value的值时，其他线程看到的value值都是最新的value值，即修改之后的volatile的值。
(02) 通过CAS设置value。这保证了：当某线程池通过CAS函数(如compareAndSet函数)设置value时，它的操作是原子的，即线程在操作value时不会被中断。

```
public class AtomicReference<V>  implements java.io.Serializable {
    private static final long serialVersionUID = -1848883965231344442L;

    // 获取Unsafe对象，Unsafe的作用是提供CAS操作
    private static final Unsafe unsafe = Unsafe.getUnsafe();
    private static final long valueOffset;

    static {
      try {
        valueOffset = unsafe.objectFieldOffset
            (AtomicReference.class.getDeclaredField("value"));
      } catch (Exception ex) { throw new Error(ex); }
    }

    // volatile类型
    private volatile V value;

    public AtomicReference(V initialValue) {
        value = initialValue;
    }

    public AtomicReference() {
    }

    public final V get() {
        return value;
    }

    public final void set(V newValue) {
        value = newValue;
    }

    public final void lazySet(V newValue) {
        unsafe.putOrderedObject(this, valueOffset, newValue);
    }

    public final boolean compareAndSet(V expect, V update) {
        return unsafe.compareAndSwapObject(this, valueOffset, expect, update);
    }

    public final boolean weakCompareAndSet(V expect, V update) {
        return unsafe.compareAndSwapObject(this, valueOffset, expect, update);
    }

    public final V getAndSet(V newValue) {
        while (true) {
            V x = get();
            if (compareAndSet(x, newValue))
                return x;
        }
    }

    public String toString() {
        return String.valueOf(get());
    }
}
```

AtomicReference示例

```
// AtomicReferenceTest.java的源码
import java.util.concurrent.atomic.AtomicReference;

public class AtomicReferenceTest {

    public static void main(String[] args){

        // 创建两个Person对象，它们的id分别是101和102。
        Person p1 = new Person(101);
        Person p2 = new Person(102);
        // 新建AtomicReference对象，初始化它的值为p1对象
        AtomicReference ar = new AtomicReference(p1);
        // 通过CAS设置ar。如果ar的值为p1的话，则将其设置为p2。
        ar.compareAndSet(p1, p2);

        Person p3 = (Person)ar.get();
        System.out.println("p3 is "+p3);
        System.out.println("p3.equals(p1)="+p3.equals(p1));
    }
}

class Person {
    volatile long id;
    public Person(long id) {
        this.id = id;
    }
    public String toString() {
        return "id:"+id;
    }
}
```

# AtomicStampedReference(解决ABA问题)
AtomicStampedReference内部不仅维护了对象值，还维护了一个时间戳（我这里把它称为时间戳，实际上它可以使任何一个整数，它使用整数来表示状态值）。当AtomicStampedReference对应的数值被修改时，除了更新数据本身外，还必须要更新时间戳。当AtomicStampedReference设置对象值时，对象值以及时间戳都必须满足期望值，写入才会成功。因此，即使对象值被反复读写，写回原值，只要时间戳发生变化，就能防止不恰当的写入。
AtomicStampedReference的几个API在AtomicReference的基础上新增了有关时间戳的信息

```
//比较设置 参数依次为：期望值 写入新值 期望时间戳 新时间戳
public boolean compareAndSet(V expectedReference,V
newReference,int expectedStamp,int newStamp)
//获得当前对象引用
public V getReference()
//获得当前时间戳
public int getStamp()
//设置当前对象引用和时间戳
public void set(V newReference, int newStamp)
```

```
private static class Pair<T> {
   final T reference;
   final int stamp;
   private Pair(T reference, int stamp) {
       this.reference = reference;
       this.stamp = stamp;
   }
   static <T> Pair<T> of(T reference, int stamp) {
       return new Pair<T>(reference, stamp);
   }
}

private volatile Pair<V> pair;


public AtomicStampedReference(V initialRef, int initialStamp) {
   pair = Pair.of(initialRef, initialStamp);
}

public void set(V newReference, int newStamp) {
        Pair<V> current = pair;
        if (newReference != current.reference || newStamp != current.stamp)
            this.pair = Pair.of(newReference, newStamp);
    }

public boolean compareAndSet(V   expectedReference,
                                 V   newReference,
                                 int expectedStamp,
                                 int newStamp) {
        Pair<V> current = pair;
        return
            expectedReference == current.reference &&
            expectedStamp == current.stamp &&
            ((newReference == current.reference &&
              newStamp == current.stamp) ||
             casPair(current, Pair.of(newReference, newStamp)));
    }

private static final sun.misc.Unsafe UNSAFE = sun.misc.Unsafe.getUnsafe();
    private static final long pairOffset =
        objectFieldOffset(UNSAFE, "pair", AtomicStampedReference.class);

    private boolean casPair(Pair<V> cmp, Pair<V> val) {
        return UNSAFE.compareAndSwapObject(this, pairOffset, cmp, val);
    }

```

1. 创建一个Pair类来记录对象引用和时间戳信息,采用int作为时间戳，实际使用的时候时间戳信息要做成自增的，否则时间戳如果重复，还会出现ABA的问题。这个Pair对象是不可变对象，所有的属性都是final的， of方法每次返回一个新的不可变对象
2. 使用一个volatile类型的引用指向当前的Pair对象，一旦volatile引用发生变化，变化对所有线程可见
3. set方法时，当要设置的对象和当前Pair对象不一样时，新建一个不可变的Pair对象
4. compareAndSet方法中，只有期望对象的引用和版本号和目标对象的引用和版本好都一样时，才会新建一个Pair对象，然后用新建的Pair对象和原理的Pair对象做CAS操作
5. 实际的CAS操作比较的是当前的pair对象和新建的pair对象，pair对象封装了引用和时间戳信息

# AtomicIntegerArray

除了提供基本数据类型外，JDK还为我们准备了数组等复合结构。当前可用的原子数组有：AtomicIntegerArray、AtomicLongArray和AtomicReferenceArray，分别表示整数数组、long型数组和普通的对象数组。

```
/*
 * ORACLE PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *

 */

/*
 *
 *
 * Written by Doug Lea with assistance from members of JCP JSR-166
 * Expert Group and released to the public domain, as explained at
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

package java.util.concurrent.atomic;
import sun.misc.Unsafe;
import java.util.*;

/**
 * An {@code int} array in which elements may be updated atomically.
 * See the {@link java.util.concurrent.atomic} package
 * specification for description of the properties of atomic
 * variables.
 * @since 1.5
 * @author Doug Lea
 */
public class AtomicIntegerArray implements java.io.Serializable {
    private static final long serialVersionUID = 2862133569453604235L;

    private static final Unsafe unsafe = Unsafe.getUnsafe();
    private static final int base = unsafe.arrayBaseOffset(int[].class);
    private static final int shift;
    private final int[] array;

    static {
        int scale = unsafe.arrayIndexScale(int[].class);
        if ((scale & (scale - 1)) != 0)
            throw new Error("data type scale not a power of two");
        shift = 31 - Integer.numberOfLeadingZeros(scale);
    }

    private long checkedByteOffset(int i) {
        if (i < 0 || i >= array.length)
            throw new IndexOutOfBoundsException("index " + i);

        return byteOffset(i);
    }

    private static long byteOffset(int i) {
        return ((long) i << shift) + base;
    }

    /**
     * Creates a new AtomicIntegerArray of the given length, with all
     * elements initially zero.
     *
     * @param length the length of the array
     */
    public AtomicIntegerArray(int length) {
        array = new int[length];
    }

    /**
     * Creates a new AtomicIntegerArray with the same length as, and
     * all elements copied from, the given array.
     *
     * @param array the array to copy elements from
     * @throws NullPointerException if array is null
     */
    public AtomicIntegerArray(int[] array) {
        // Visibility guaranteed by final field guarantees
        this.array = array.clone();
    }

    /**
     * Returns the length of the array.
     *
     * @return the length of the array
     */
    public final int length() {
        return array.length;
    }

    /**
     * Gets the current value at position {@code i}.
     *
     * @param i the index
     * @return the current value
     */
    public final int get(int i) {
        return getRaw(checkedByteOffset(i));
    }

    private int getRaw(long offset) {
        return unsafe.getIntVolatile(array, offset);
    }

    /**
     * Sets the element at position {@code i} to the given value.
     *
     * @param i the index
     * @param newValue the new value
     */
    public final void set(int i, int newValue) {
        unsafe.putIntVolatile(array, checkedByteOffset(i), newValue);
    }

    /**
     * Eventually sets the element at position {@code i} to the given value.
     *
     * @param i the index
     * @param newValue the new value
     * @since 1.6
     */
    public final void lazySet(int i, int newValue) {
        unsafe.putOrderedInt(array, checkedByteOffset(i), newValue);
    }

    /**
     * Atomically sets the element at position {@code i} to the given
     * value and returns the old value.
     *
     * @param i the index
     * @param newValue the new value
     * @return the previous value
     */
    public final int getAndSet(int i, int newValue) {
        long offset = checkedByteOffset(i);
        while (true) {
            int current = getRaw(offset);
            if (compareAndSetRaw(offset, current, newValue))
                return current;
        }
    }

    /**
     * Atomically sets the element at position {@code i} to the given
     * updated value if the current value {@code ==} the expected value.
     *
     * @param i the index
     * @param expect the expected value
     * @param update the new value
     * @return true if successful. False return indicates that
     * the actual value was not equal to the expected value.
     */
    public final boolean compareAndSet(int i, int expect, int update) {
        return compareAndSetRaw(checkedByteOffset(i), expect, update);
    }

    private boolean compareAndSetRaw(long offset, int expect, int update) {
        return unsafe.compareAndSwapInt(array, offset, expect, update);
    }

    /**
     * Atomically sets the element at position {@code i} to the given
     * updated value if the current value {@code ==} the expected value.
     *
     * <p>May <a href="package-summary.html#Spurious">fail spuriously</a>
     * and does not provide ordering guarantees, so is only rarely an
     * appropriate alternative to {@code compareAndSet}.
     *
     * @param i the index
     * @param expect the expected value
     * @param update the new value
     * @return true if successful.
     */
    public final boolean weakCompareAndSet(int i, int expect, int update) {
        return compareAndSet(i, expect, update);
    }

    /**
     * Atomically increments by one the element at index {@code i}.
     *
     * @param i the index
     * @return the previous value
     */
    public final int getAndIncrement(int i) {
        return getAndAdd(i, 1);
    }

    /**
     * Atomically decrements by one the element at index {@code i}.
     *
     * @param i the index
     * @return the previous value
     */
    public final int getAndDecrement(int i) {
        return getAndAdd(i, -1);
    }

    /**
     * Atomically adds the given value to the element at index {@code i}.
     *
     * @param i the index
     * @param delta the value to add
     * @return the previous value
     */
    public final int getAndAdd(int i, int delta) {
        long offset = checkedByteOffset(i);
        while (true) {
            int current = getRaw(offset);
            if (compareAndSetRaw(offset, current, current + delta))
                return current;
        }
    }

    /**
     * Atomically increments by one the element at index {@code i}.
     *
     * @param i the index
     * @return the updated value
     */
    public final int incrementAndGet(int i) {
        return addAndGet(i, 1);
    }

    /**
     * Atomically decrements by one the element at index {@code i}.
     *
     * @param i the index
     * @return the updated value
     */
    public final int decrementAndGet(int i) {
        return addAndGet(i, -1);
    }

    /**
     * Atomically adds the given value to the element at index {@code i}.
     *
     * @param i the index
     * @param delta the value to add
     * @return the updated value
     */
    public final int addAndGet(int i, int delta) {
        long offset = checkedByteOffset(i);
        while (true) {
            int current = getRaw(offset);
            int next = current + delta;
            if (compareAndSetRaw(offset, current, next))
                return next;
        }
    }

    /**
     * Returns the String representation of the current values of array.
     * @return the String representation of the current values of array
     */
    public String toString() {
        int iMax = array.length - 1;
        if (iMax == -1)
            return "[]";

        StringBuilder b = new StringBuilder();
        b.append('[');
        for (int i = 0; ; i++) {
            b.append(getRaw(byteOffset(i)));
            if (i == iMax)
                return b.append(']').toString();
            b.append(',').append(' ');
        }
    }

}
```

这里以AtomicIntegerArray为例，展示原子数组的使用方式。

```
public class AtomicIntegerArrayDemo {
   staticAtomicIntegerArray arr = new AtomicIntegerArray(10);
     public staticclass AddThread implements Runnable{
         publicvoid run(){
            for(intk=0;k<10000;k++)
                 arr.getAndIncrement(k%arr.length());
         }
     }
    public staticvoid main(String[] args) throws InterruptedException {
         Thread[]ts=new Thread[10];
         for(intk=0;k<10;k++){
            ts[k]=new Thread(new AddThread());
         }
         for(intk=0;k<10;k++){ts[k].start();}
         for(intk=0;k<10;k++){ts[k].join();}
         System.out.println(arr);
    }
}
```

# AtomicIntegerFieldUpdater

AtomicIntegerFieldUpdater就是用来更新某一个实例对象里面的int属性的。
但是注意，在用法上有规则：


字段必须是volatile类型的，在线程之间共享变量时保证立即可见
字段的描述类型（修饰符public/protected/default/private）是与调用者与操作对象字段的关系一致。也就是说调用者能够直接操作对象字段，那么就可以反射进行原子操作。
对于父类的字段，子类是不能直接操作的，尽管子类可以访问父类的字段。
只能是实例变量，不能是类变量，也就是说不能加static关键字。
只能是可修改变量，不能使final变量，因为final的语义就是不可修改。
对于AtomicIntegerFieldUpdater和AtomicLongFieldUpdater只能修改int/long类型的字段，不能修改其包装类型（Integer/Long）。如果要修改包装类型就需要使用AtomicReferenceFieldUpdater。


具体规则可以通过以下测试例子来分析：

```
public class AtomicIntegerFieldUpdaterAnalyzeTest {

    public static void main(String[] args) {
        AtomicIntegerFieldUpdaterAnalyzeTest test = new AtomicIntegerFieldUpdaterAnalyzeTest();
        test.testValue();
    }

    public AtomicIntegerFieldUpdater<DataDemo> updater(String name) {
        return AtomicIntegerFieldUpdater.newUpdater(DataDemo.class, name);
    }

    public void testValue() {
        DataDemo data = new DataDemo();
//      //访问父类的public 变量，报错：java.lang.NoSuchFieldException
//      System.out.println("fatherVar = "+updater("fatherVar").getAndIncrement(data));
//
//      //访问普通 变量，报错：java.lang.IllegalArgumentException: Must be volatile type
//      System.out.println("intVar = "+updater("intVar").getAndIncrement(data));

//      //访问public volatile int 变量，成功
//      System.out.println("publicVar = "+updater("publicVar").getAndIncrement(data));
//
//      //访问protected volatile int 变量，成功
//      System.out.println("protectedVar = "+updater("protectedVar").getAndIncrement(data));
//
//      //访问其他类private volatile int变量，失败：java.lang.IllegalAccessException
//      System.out.println("privateVar = "+updater("privateVar").getAndIncrement(data));
//
//      //访问，static volatile int，失败，只能访问实例对象：java.lang.IllegalArgumentException
//      System.out.println("staticVar = "+updater("staticVar").getAndIncrement(data));
//
//      //访问integer变量，失败， Must be integer type
//      System.out.println("integerVar = "+updater("integerVar").getAndIncrement(data));
//
//      //访问long 变量，失败， Must be integer type
//      System.out.println("longVar = "+updater("longVar").getAndIncrement(data));

        //自己在自己函数里面可以访问自己的private变量，所以如果可见，那么可以进行原子性字段更新
        data.testPrivate();
    }
}
class Father{
    public volatile int fatherVar = 4;
}
class DataDemo extends Father {

    public int intVar = 4;

    public volatile int publicVar = 3;
    protected volatile int protectedVar = 4;
    private volatile int privateVar = 5;

    public volatile static int staticVar = 10;
    //The field finalVar can be either final or volatile, not both
    //public final volatile int finalVar = 11;

    public volatile Integer integerVar = 19;
    public volatile Long longVar = 18L;

    public void testPrivate(){
        DataDemo data = new DataDemo();
        System.out.println(AtomicIntegerFieldUpdater.newUpdater(DataDemo.class, "privateVar").getAndIncrement(data));
    }
}
```

实现
首先看定义：

```
/**
 * 允许一个已经定义的类里面的某一个volatile int型变量的原子更新。
 * 注意只能够原子更新里面的某一个int型的变量。
 * 思路是通过反射获取变量，为一个updater，然后进行更新。
 */
public abstract class AtomicIntegerFieldUpdater<T>
```

AtomicIntegerFieldUpdater是一个抽象类，但是它内部有一个private final类型的默认子类，所以在调用newUpdater的时候，会用模式子类来实现：

```
/**
     * 创建一个updater，
     * tclass：包含类的名称
     * fieldName：字段名字
     */
    @CallerSensitive
    public static <U> AtomicIntegerFieldUpdater<U> newUpdater(Class<U> tclass,
                                                              String fieldName) {
        return new AtomicIntegerFieldUpdaterImpl<U>
            (tclass, fieldName, Reflection.getCallerClass());
    }
```

而除了这些之外，子类中还有判断对象访问权限，以及判断是否为父类，是否同一个包等方法：

```
//判断second是否为first的父类
private static boolean isAncestor(ClassLoader first, ClassLoader second) ;
//判断class1和class2是否在同一个包下
private static boolean isSamePackage(Class<?> class1, Class<?> class2)
//获得包名
private static String getPackageName(Class<?> cls)
//判断object是否为当前class的一个子类
private final void accessCheck(T obj)
```

另外就是一些CAS方法，实际上都是调用Unsafe.java中的native方法：

```
public final boolean compareAndSet(T obj, int expect, int update) {
            accessCheck(obj);
            return U.compareAndSwapInt(obj, offset, expect, update);
        }

        public final boolean weakCompareAndSet(T obj, int expect, int update) {
            accessCheck(obj);
            return U.compareAndSwapInt(obj, offset, expect, update);
        }

        public final void set(T obj, int newValue) {
            accessCheck(obj);
            U.putIntVolatile(obj, offset, newValue);
        }

        public final void lazySet(T obj, int newValue) {
            accessCheck(obj);
            U.putOrderedInt(obj, offset, newValue);
        }

        public final int get(T obj) {
            accessCheck(obj);
            return U.getIntVolatile(obj, offset);
        }

        public final int getAndSet(T obj, int newValue) {
            accessCheck(obj);
            return U.getAndSetInt(obj, offset, newValue);
        }
```

AtomicLongFieldUpdater和AtomicReferenceFieldUpdater

在AtomicLongFieldUpdater类中，由于有些32位系统一次性无法对64位的long进行原子运算，所以为了保证安全，在这些不能一次性进行原子运算的需要区分考虑，利用加synchronized锁来实现：

```
@CallerSensitive
    public static <U> AtomicLongFieldUpdater<U> newUpdater(Class<U> tclass,
                                                           String fieldName) {
        Class<?> caller = Reflection.getCallerClass();
        if (AtomicLong.VM_SUPPORTS_LONG_CAS)
        //直接cas实现
            return new CASUpdater<U>(tclass, fieldName, caller);
        else
        //带synchronized锁实现
            return new LockedUpdater<U>(tclass, fieldName, caller);
    }
```
