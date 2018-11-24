# Spring AOP

AOP（Aspect Orient Programming），我们一般称为面向方面（切面）编程，作为面向对象的一种补充，用于处理系统中分布于各个模块的横切关注点，比如事务管理、日志、缓存等等。

AOP实现的关键在于AOP框架自动创建的AOP代理

AOP代理主要分为
(1)静态代理,代表为AspectJ；
(2)动态代理，以Spring AOP为代表。

# 静态代理AspectJ

使用AspectJ的编译时增强实现AOP

AspectJ是静态代理的增强，所谓的静态代理就是AOP框架会在编译阶段生成AOP代理类，因此也称为编译时增强。

举个实例的例子来说。首先我们有一个普通的Hello类

```
public class Hello {
    public void sayHello() {
        System.out.println("hello");
    }

    public static void main(String[] args) {
        Hello h = new Hello();
        h.sayHello();
    }
}
```

使用AspectJ编写一个Aspect

```
public aspect TxAspect {
    void around():call(void Hello.sayHello()){
        System.out.println("开始事务 ...");
        proceed();
        System.out.println("事务结束 ...");
    }
}
```

这里模拟了一个事务的场景，类似于Spring的声明式事务。使用AspectJ的编译器编译

```
ajc -d . Hello.java TxAspect.aj
```

编译完成之后再运行这个Hello类，可以看到以下输出

```
开始事务 ...
hello
事务结束 ...
```

显然，AOP已经生效了，那么究竟AspectJ是如何在没有修改Hello类的情况下为Hello类增加新功能的呢？

查看一下编译后的Hello.class

```
public class Hello {
    public Hello() {
    }

    public void sayHello() {
        System.out.println("hello");
    }

    public static void main(String[] args) {
        Hello h = new Hello();
        sayHello_aroundBody1$advice(h, TxAspect.aspectOf(), (AroundClosure)null);
    }
}
```

可以看到，这个类比原来的Hello.java多了一些代码，这就是AspectJ的静态代理，它会在编译阶段将Aspect织入Java字节码中， 运行的时候就是经过增强之后的AOP对象。

```
public void ajc$around$com_listenzhangbin_aop_TxAspect$1$f54fe983(AroundClosure ajc$aroundClosure) {
        System.out.println("开始事务 ...");
        ajc$around$com_listenzhangbin_aop_TxAspect$1$f54fe983proceed(ajc$aroundClosure);
        System.out.println("事务结束 ...");
    }
```

从Aspect编译后的class文件可以更明显的看出执行的逻辑。proceed方法就是回调执行被代理类中的方法。

# 动态代理Spring AOP

与AspectJ的静态代理不同，Spring AOP使用的动态代理

所谓的动态代理就是说AOP框架不会去修改字节码，而是在内存中临时为方法生成一个AOP对象，这个AOP对象包含了目标对象的全部方法，并且在特定的切点做了增强处理，并回调原对象的方法。

Spring AOP中的动态代理主要有两种方式

（1）JDK动态代理通过反射来接收被代理的类，并且要求被代理的类必须实现一个接口。JDK动态代理的核心是InvocationHandler接口和Proxy类。

（2）如果目标类没有实现接口，那么Spring AOP会选择使用CGLIB来动态代理目标类。CGLIB（Code Generation Library），是一个代码生成的类库，可以在运行时动态的生成某个类的子类，注意，CGLIB是通过继承的方式做的动态代理，因此如果某个类被标记为final，那么它是无法使用CGLIB做动态代理的。

为了验证以上的说法，可以做一个简单的测试。首先测试实现接口的情况。

定义一个接口

```
public interface Person {
    String sayHello(String name);
}
```

实现类

```
@Component
public class Chinese implements Person {

    @Timer
    @Override
    public String sayHello(String name) {
        System.out.println("-- sayHello() --");
        return name + " hello, AOP";
    }

    public void eat(String food) {
        System.out.println("我正在吃：" + food);
    }

}
```

这里的@Timer注解是我自己定义的一个普通注解，用来标记Pointcut。

定义Aspect

```
@Aspect
@Component
public class AdviceTest {

    @Pointcut("@annotation(com.listenzhangbin.aop.Timer)")
    public void pointcut() {
    }

    @Before("pointcut()")
    public void before() {
        System.out.println("before");
    }
}
```

运行

```
@SpringBootApplication
@RestController
public class SpringBootDemoApplication {

    //这里必须使用Person接口做注入
    @Autowired
    private Person chinese;

    @RequestMapping("/test")
    public void test() {
        chinese.sayHello("listen");
        System.out.println(chinese.getClass());
    }

    public static void main(String[] args) {
        SpringApplication.run(SpringBootDemoApplication.class, args);
    }
}
```

输出

```
before
-- sayHello() --
class com.sun.proxy.$Proxy53
```

可以看到类型是com.sun.proxy.$Proxy53，也就是前面提到的Proxy类，因此这里Spring AOP使用了JDK的动态代理。

再来看看不实现接口的情况，修改Chinese类

```
@Component
public class Chinese {

    @Timer
//    @Override
    public String sayHello(String name) {
        System.out.println("-- sayHello() --");
        return name + " hello, AOP";
    }

    public void eat(String food) {
        System.out.println("我正在吃：" + food);
    }

}
```

运行

```
SpringBootApplication
@RestController
public class SpringBootDemoApplication {

    //直接用Chinese类注入
    @Autowired
    private Chinese chinese;

    @RequestMapping("/test")
    public void test() {
        chinese.sayHello("listen");
        System.out.println(chinese.getClass());
    }

    public static void main(String[] args) {
        SpringApplication.run(SpringBootDemoApplication.class, args);
    }
}
```

输出

```
before
-- sayHello() --
class com.listenzhangbin.aop.Chinese$$EnhancerBySpringCGLIB$$56b89168
```

可以看到类被CGLIB增强了，也就是动态代理。这里的CGLIB代理就是Spring AOP的代理，这个类也就是所谓的AOP代理，AOP代理类在切点动态地织入了增强处理。

小结
AspectJ在编译时就增强了目标对象，Spring AOP的动态代理则是在每次运行时动态的增强，生成AOP代理对象，区别在于生成AOP代理对象的时机不同，相对来说AspectJ的静态代理方式具有更好的性能，但是AspectJ需要特定的编译器进行处理，而Spring AOP则无需特定的编译器处理。
