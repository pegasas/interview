# 设计模式

## 1 单例模式

单例模式，单例对象的类必须保证只有一个实例存在。

单例模式的关键点
1) 构造方法不对外开放,为private
2) 确保单例类只有一个对象,尤其是多线程模式下
3) 通过静态方法或枚举返回单例对象
4) 确保单例类在反序列化是不会重新创建新的对象

5种实现方式：

（1）饿汉模式

```
public class Singleton1 {
    /*
    * 饿汉式是在声明的时候就已经初始化Singleton1,确保了对象的唯一性
    *
    * 声明的时候就初始化对象会浪费不必要的资源
    * */
    private static Singleton1 instance = new Singleton1();

    private Singleton1() {
    }

    // 通过静态方法或枚举返回单例对象
    public Singleton1 getInstance() {
        return instance;
    }
}
```

（2）懒汉模式

```
public class Singleton2 {

    private static Singleton2 instance;

    private Singleton2() {
    }

    /*
    * getInstance 添加了synchronized 关键字,,也就是说 getInstance 是一个同步方法,
    * 这就是懒汉式在多线程中保持唯一性的方式
    *
    * 懒汉式存在的问题是,即是instance已经被创建,每次调用getInstance方法还是会同步,这样就会浪费一些不必要的资源
    * */
    public static synchronized Singleton2 getInstance() {
        if (instance == null) {
            instance = new Singleton2();
        }
        return instance;
    }
}
```
（3）Double Check Lock(DCL模式)

```
private static Singleton3 instance;

    private Singleton3() {
    }

    /**
     * getInstance 进行了两次判空,第一次判空是为了不必要的同步,第二次判空为了在instance 为 null 的情况下创建实例
     * 既保证了线程安全且单例对象初始化后调用getInstance又不会进行同步锁判断
     * <p>
     * 优点:资源利用率高,效率高
     * 缺点:第一次加载稍慢,由于java处理器允许乱序执行,偶尔会失败
     *
     * @return
     */
    public static Singleton3 getInstance() {
        if (instance == null) {
            synchronized (Singleton3.class) {
                if (instance == null) {
                    instance = new Singleton3();
                }
            }
        }
        return instance;
    }
}
```

（4）静态内部类

```
public class Singleton4 {
    /*
    *当第一次加载Singleton类时并不会初始化SINGLRTON,只有第一次调用getInstance方法的时候才会初始化SINGLETON
    *第一次调用getInstance 方法的时候虚拟机才会加载SingletonHoder类,这种方式不仅能够保证线程安全,也能够保证对象的唯一,
    *还延迟了单例的实例化,所有推荐使用这种方式
    * */
    private Singleton4() {
    }

    public Singleton4 getInstance() {
        return SingletonHolder.SINGLETON;
    }

    private static class SingletonHolder {
        private static final Singleton4 SINGLETON = new Singleton4();
    }
}
```

（5）枚举实现

```
public enum SingletonEnum {
    /**
     *枚举是写法最简单的
     * 默认枚举实例的创建时线程安全的,且在任何一种情况下它都是单例,包括反序列化
     * */
    INSTANCE;
}
```
（6）容器实现

```
public class Singleton5 {
    /**
     * 将多种类型的单例放到统一的Map将集合中,根据相应的Key获取对应的对象
     *
     * 这种方式是我们可以管理多种类型的单例,可以使用统一接口进行获取操作
     * 降低了使用成本,也隐藏了具体实现,降低了耦合度
     */
    public static Map<String, Object> objMap = new HashMap<String, Object>();

    private Singleton5() {
    }

    public static void registerInstance(String key, Object instance) {
        if (!objMap.containsKey(key)) {
            objMap.put(key, instance);
        }
    }

    public static Object getInstance(String key) {
        return objMap.get(key);
    }
}
```

反序列化时会重新生成对象的问题

已上几种方法里面 枚举任何一种情况都是单例,不存在这个问题,容器模式只是承载单例的容器所以它本身也不存在这个问题,只有其他三种方式种存在这个反序列话的问题,怎么解决呢?加入如下代码

```
// 防止反序列化获取多个对象的漏洞
// 实现Serializable接口，当从I/O流中读取对象时，readResolve()方法都会被调用到
// 用readResolve()中返回的对象直接替换在反序列化过程中创建的对象
private Singleton4 readResolve() throws ObjectStreamException {
    return SingletonHolder.SINGLETON;
}
```
