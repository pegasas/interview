# static关键字

1.修饰成员变量:

给属性加了static关键字之后，对象就不再拥有属性了，属性会统一交给P类去管理，即多个对象只会对应一个属性，一个对象如果对属性做了改变，其他的对象都会受到影响。

一般用法：private static

2.修饰成员方法

可以使用"类名.方法名"的方式操作方法，避免了先要new出对象的繁琐和资源消耗

3.修饰静态块

当我们初始化static修饰的成员时，可以将他们统一放在一个以static开始，用花括号包裹起来的块状语句中

4.静态导包

静态导包用法，将类的方法直接导入到当前类中，从而直接使用“方法名”即可调用类方法，更加方便。
