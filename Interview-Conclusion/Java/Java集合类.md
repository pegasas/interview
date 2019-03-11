## 4 Java集合类

### 4.1 快速失败(fail-fast) && 安全失败(fail-safe)

快速失败(fail-fast)：当你在迭代一个集合的时候，如果有另一个线程正在修改你正在访问的那个集合时，就会抛出一个ConcurrentModification异常。
在java.util包下的都是快速失败。

安全失败(fail-safe)：你在迭代的时候会去底层集合做一个拷贝，所以你在修改上层集合的时候是不会受影响的，不会抛出ConcurrentModification异常。
在java.util.concurrent包下的全是安全失败的。

### 4.2 HashTable

Hashtable 也是一个散列表，它存储的内容是键值对(key-value)映射。
Hashtable 继承于Dictionary，实现了Map、Cloneable、java.io.Serializable接口。
Hashtable 是线程安全的。
Hashtable 的key、value都不可以为null。
Hashtable中的映射不是有序的。

Hashtable 的实例有两个参数影响其性能：初始容量 和 加载因子。容量 是哈希表中桶 的数量，初始容量 就是哈希表创建时的容量。注意，哈希表的状态为 open：在发生“哈希冲突”的情况下，单个桶会存储多个条目，这些条目必须按顺序搜索。加载因子 是对哈希表在其容量自动增加之前可以达到多满的一个尺度。初始容量和加载因子这两个参数只是对该实现的提示。关于何时以及是否调用 rehash 方法的具体细节则依赖于该实现。

Hashtable是通过"拉链法"实现的哈希表。它包括几个重要的成员变量：table, count, threshold, loadFactor, modCount。

table是一个Entry[]数组类型，而Entry实际上就是一个单向链表。哈希表的"key-value键值对"都是存储在Entry数组中的。
count是Hashtable的大小，它是Hashtable保存的键值对的数量。
threshold是Hashtable的阈值，用于判断是否需要调整Hashtable的容量。threshold的值="容量*加载因子"。
loadFactor就是加载因子。
modCount是用来实现fail-fast机制的

![avatar][HashTable]

### 4.3 HashMap

#### 4.3.1 Java7 HashMap

![avatar][Java7HashMap]

HashMap 里面是一个数组，然后数组中每个元素是一个单向链表。

上图中，每个绿色的实体是嵌套类 Entry 的实例，Entry 包含四个属性：key, value, hash 值和用于单向链表的 next。

capacity：当前数组容量，始终保持 2^n，可以扩容，扩容后数组大小为当前的 2 倍。

loadFactor：负载因子，默认为 0.75。

threshold：扩容的阈值，等于 capacity * loadFactor

#### 4.3.2 Java8 HashMap

![avatar][Java8HashMap]

在 Java8 中，当链表中的元素超过了 8 个以后，会将链表转换为红黑树，在这些位置进行查找的时候可以降低时间复杂度为 O(logN)。

Java7 中使用 Entry 来代表每个 HashMap 中的数据节点，Java8 中使用 Node，基本没有区别，都是 key，value，hash 和 next 这四个属性，不过，Node 只能用于链表的情况，红黑树的情况需要使用 TreeNode。

我们根据数组元素中，第一个节点数据类型是 Node 还是 TreeNode 来判断该位置下是链表还是红黑树的。

#### 4.3.3 HashMap Resize

##### 4.3.3.1 java7 HashMap Resize

HashMap的底层数组长度总是2的n次方，在构造函数中存在：capacity <<= 1;这样做总是能够保证HashMap的底层数组长度为2的n次方。当length为2的n次方时，h&(length - 1)就相当于对length取模，而且速度比直接取模快得多，这是HashMap在速度上的一个优化。

而当length = 2的n次方时，length – 1 = n个2进制1，那么进行低位&运算时，值总是与原来hash值相同，而进行高位运算时，其值等于其低位值。不需要像JDK1.7的实现那样重新计算hash，只需要看看原来的hash值新增的那个bit是1还是0就好了，是0的话索引没变，是1的话索引变成“原索引+oldCap”，由于新增的1bit是0还是1可以认为是随机的，因此resize的过程，均匀的把之前的冲突的节点分散到新的bucket了。
所以说当length = 2^n时，不同的hash值发生碰撞的概率比较小，这样就会使得数据在table数组中分布较均匀，查询速度也较快。

##### 4.3.3.2 java8 HashMap Resize

JDK7是每拿到一个Node就直接插入到newTable，而JDK8是先插入到高低链表中，然后再一次性插入到newTable。
所以链表的扩容过程JDK7会出现死循环问题，而JDK8避免了这个问题。
JDK8跟原先的链表对比Node之间顺序是一致的，而JDK7是是反过来的。

#### 4.3.4 HashMap线程不安全原因

1.多线程resize发生死循环死锁
2.使用迭代器的过程中有其他线程修改了map，那么将抛出ConcurrentModificationException，这就是所谓fail-fast策略。

#### 4.3.5 3种线程安全HashMap

Hashtable:使用synchronized来保证线程安全的
ConcurrentHashMap:分segment上锁
Synchronized Map会返回一个SynchronizedMap类的对象，而在SynchronizedMap类中使用了synchronized同步关键字来保证对Map的操作是线程安全的

```
//Hashtable
Map<String, String> hashtable = new Hashtable<>();

//synchronizedMap
Map<String, String> synchronizedHashMap = Collections.synchronizedMap(new HashMap<String, String>());

//ConcurrentHashMap
Map<String, String> concurrentHashMap = new ConcurrentHashMap<>();
```

#### 4.3.6 HashSet

|HashMap|HashSet|
|--|--|
|实现了Map接口|实现Set接口|
|存储键值对|仅存储对象|
|调用put（）向map中添加元素|调用add（）方法向Set中添加元素|
|HashMap使用键（Key）计算Hashcode|HashSet使用成员对象来计算hashcode值，

对于两个对象来说hashcode可能相同，

所以equals()方法用来判断对象的相等性，

如果两个对象不同的话，那么返回false|
|HashMap相对于HashSet较快，因为它是使用唯一的键获取对象|HashSet较HashMap来说比较慢|

### 4.4 LinkedHashMap && LinkedHashSet

LinkedHashSet和LinkedHashMap在Java里也有着相同的实现，前者仅仅是对后者做了一层包装，也就是说LinkedHashSet里面有一个LinkedHashMap（适配器模式）。

LinkedHashMap实现了Map接口，即允许放入key为null的元素，也允许插入value为null的元素。LinkedHashMap 在HashMap的基础上，增加了一条双向链表LinkedList，使得上面的结构可以保持键值对的插入顺序。同时通过对链表进行相应的操作，实现了访问顺序相关逻辑。其结构如下：

![avatar][LinkedHashMap]

### 4.5 TreeMap && TreeSet

TreeMap和TreeSet在Java里也有着相同的实现，前者仅仅是对后者做了一层包装，也就是说TreeSet里面有一个TreeMap（适配器模式）。

TreeMap基于红黑树（Red-Black tree）实现。该映射根据其键的自然顺序进行排序，或者根据创建映射时提供的 Comparator 进行排序，具体取决于使用的构造方法。
TreeMap是非同步的。 它的iterator 方法返回的迭代器是fail-fast的。

### 4.6 PriortityQueue

PriorityQueue队列基于最小堆原理实现

### 4.7 EnumMap && EnumSet

EnumMap是专门为枚举类型量身定做的Map实现。虽然使用其它的Map实现（如HashMap）也能完成枚举类型实例到值得映射，但是使用EnumMap会更加高效：它只能接收同一枚举类型的实例作为键值，并且由于枚举类型实例的数量相对固定并且有限，所以EnumMap使用数组来存放与枚举类型对应的值。这使得EnumMap的效率非常高。EnumMap在内部使用枚举类型的ordinal()得到当前实例的声明次序，并使用这个次序维护枚举类型实例对应值在数组的位置。

EnumMap的基本实现原理，内部有两个数组，长度相同，一个表示所有可能的键，一个表示对应的值，值为null表示没有该键值对，键都有一个对应的索引，根据索引可直接访问和操作其键和值，效率很高。

### 4.8 IdentityHashMap

IdentityHashMap允许key值重复，但是key必须是两个不同的对象，即对于k1和k2，当k1==k2时，IdentityHashMap认为两个key相等，而HashMap只有在k1.equals(k2) == true 时才会认为两个key相等。

IdentityHashMap不是Map的通用实现，它有意违反了Map的常规协定。

IdentityHashMap允许key和value都为null。

IdentityHashMap也是无序的，并且该类不是线程安全的。

在key-value数据的存储上，类似于HashMap，采用map数组进行存储，但是key-value不是利用链表解决冲突，而是继续计算下一个索引，把数据计算在下一个有效索引的数组中，也就是数据全部存储map数组中，并且table[i]=key 则table[i+1]=value。key-value紧挨着存储在map数组中。

### 4.9 WeakHashMap

新建WeakHashMap，将“键值对”添加到WeakHashMap中。实际上，WeakHashMap是通过数组table保存Entry(键值对)；每一个Entry实际上是一个单向链表，即Entry是键值对链表
当某“弱键”不再被其它对象引用，并被GC回收时。在GC回收该“弱键”时，这个“弱键”也同时会被添加到ReferenceQueue(queue)队列中
当下一次我们需要操作WeakHashMap时，会先同步table和queue。table中保存了全部的键值对，而queue中保存被GC回收的键值对；同步它们，就是删除table中被GC回收的键值对。
这就是“弱键”如何被自动从WeakHashMap中删除的步骤了。和HashMap一样，WeakHashMap是不同步的。可以使用 Collections.synchronizedMap 方法来构造同步的 WeakHashMap。

### 4.10 ArrayList

ArrayList可以简单的看作是动态数组，相对于普通的数组它可以动态的增加容量或者减少容量。要注意的是ArrayList并不是线程安全的，因此一般建议在单线程中使用ArrayList。

### 4.11 ArrayDeque

ArrayDeque 是 Deque 接口的一种具体实现，是依赖于循环队列来实现的。ArrayDeque 没有容量限制，可根据需求自动进行扩容。ArrayDeque不支持值为 null 的元素。
