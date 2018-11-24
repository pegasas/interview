# final关键字

final根据修饰位置的不同作用也不相同，针对三种情况：

1）修饰变量，被final修饰的变量必须要初始化，赋初值后不能再重新赋值。

注意：局部变量不在我们讨论的范畴，因为局部变量本身就有作用范围，不使用private、public等词修饰。

2）修饰方法，被final修饰的方法代表不能重写。

3）修饰类，被final修饰的类，不能够被继承。

注意：final修饰的类，类中的所有成员方法都被隐式地指定为final方法。

# final和static的区别

static作用于成员变量用来表示只保存一份副本，而final的作用是用来保证变量不可变

```
public class Test {
    public static void main(String[] args)  {
        MyClass myClass1 = new MyClass();
        MyClass myClass2 = new MyClass();
        System.out.println(myClass1.i);
        System.out.println(myClass1.j);
        System.out.println(myClass2.i);
        System.out.println(myClass2.j);

    }
}
class MyClass {
    public final double i = Math.random();
    public static double j = Math.random();
}
//运行结果，两次打印，j的值都是一样的，j是static类型的属于类，因此两次值相同。i不是static的因此属于对象，但是i的值是不可变的。
```
